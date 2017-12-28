/*
 * Copyright 2017-2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include "leveldb/env.h"
#include "port/port_posix.h"
#include "histogram.h"
#include "mutexlock.h"
#include "random.h"
#include "pmemkv.h"

static const string USAGE =
        "pmemkv_bench\n"
                "--engine=<name>            (storage engine name, default: kvtree)\n"
                "--db=<location>            (path to persistent pool, default: /dev/shm/pmemkv)\n"
                "--db_size_in_gb=<integer>  (size of persistent pool in GB, default: 1)\n"
                "--histogram=<0|1>          (show histograms when reporting latencies)\n"
                "--num=<integer>            (number of keys to place in database, default: 1000000)\n"
                "--reads=<integer>          (number of read operations, default: 1000000)\n"
                "--threads=<integer>        (number of concurrent threads, default: 1)\n"
                "--value_size=<integer>     (size of values in bytes, default: 100)\n"
                "--benchmarks=<name>,       (comma-separated list of benchmarks to run)\n"
                "    fillseq                (load N values in sequential key order into fresh db)\n"
                "    fillrandom             (load N values in random key order into fresh db)\n"
                "    overwrite              (replace N values in random key order)\n"
                "    readseq                (read N values in sequential key order)\n"
                "    readrandom             (read N values in random key order)\n"
                "    readmissing            (read N missing values in random key order)\n"
                "    deleteseq              (delete N values in sequential key order)\n"
                "    deleterandom           (delete N values in random key order)\n";

// Default list of comma-separated operations to run
static const char *FLAGS_benchmarks =
        "fillrandom,overwrite,fillseq,readrandom,readseq,readrandom,readmissing,readrandom,deleteseq";

// Default engine name
static const char *FLAGS_engine = "kvtree";

// Number of key/values to place in database
static int FLAGS_num = 1000000;

// Number of read operations to do.  If negative, do FLAGS_num reads.
static int FLAGS_reads = -1;

// Number of concurrent threads to run.
static int FLAGS_threads = 1;

// Size of each value
static int FLAGS_value_size = 100;

// Print histogram of operation timings
static bool FLAGS_histogram = false;

// Use the db with the following name.
static const char *FLAGS_db = NULL;

// Use following size when opening the database.
static int FLAGS_db_size_in_gb = 1;

using namespace leveldb;

leveldb::Env *g_env = NULL;

#if defined(__linux)

static Slice TrimSpace(Slice s) {
    size_t start = 0;
    while (start < s.size() && isspace(s[start])) {
        start++;
    }
    size_t limit = s.size();
    while (limit > start && isspace(s[limit - 1])) {
        limit--;
    }
    return Slice(s.data() + start, limit - start);
}

#endif

static void AppendWithSpace(std::string *str, Slice msg) {
    if (msg.empty()) return;
    if (!str->empty()) {
        str->push_back(' ');
    }
    str->append(msg.data(), msg.size());
}

class Stats {
private:
    double start_;
    double finish_;
    double seconds_;
    int done_;
    int next_report_;
    int64_t bytes_;
    double last_op_finish_;
    Histogram hist_;
    std::string message_;

public:
    Stats() { Start(); }

    void Start() {
        next_report_ = 100;
        last_op_finish_ = start_;
        hist_.Clear();
        done_ = 0;
        bytes_ = 0;
        seconds_ = 0;
        start_ = g_env->NowMicros();
        finish_ = start_;
        message_.clear();
    }

    void Merge(const Stats &other) {
        hist_.Merge(other.hist_);
        done_ += other.done_;
        bytes_ += other.bytes_;
        seconds_ += other.seconds_;
        if (other.start_ < start_) start_ = other.start_;
        if (other.finish_ > finish_) finish_ = other.finish_;

        // Just keep the messages from one thread
        if (message_.empty()) message_ = other.message_;
    }

    void Stop() {
        finish_ = g_env->NowMicros();
        seconds_ = (finish_ - start_) * 1e-6;
    }

    void AddMessage(Slice msg) {
        AppendWithSpace(&message_, msg);
    }

    void FinishedSingleOp() {
        if (FLAGS_histogram) {
            double now = g_env->NowMicros();
            double micros = now - last_op_finish_;
            hist_.Add(micros);
            if (micros > 20000) {
                fprintf(stderr, "long op: %.1f micros%30s\r", micros, "");
                fflush(stderr);
            }
            last_op_finish_ = now;
        }

        done_++;
        if (done_ >= next_report_) {
            if (next_report_ < 1000) next_report_ += 100;
            else if (next_report_ < 5000) next_report_ += 500;
            else if (next_report_ < 10000) next_report_ += 1000;
            else if (next_report_ < 50000) next_report_ += 5000;
            else if (next_report_ < 100000) next_report_ += 10000;
            else if (next_report_ < 500000) next_report_ += 50000;
            else next_report_ += 100000;
            fprintf(stderr, "... finished %d ops%30s\r", done_, "");
            fflush(stderr);
        }
    }

    void AddBytes(int64_t n) {
        bytes_ += n;
    }

    void Report(const Slice &name) {
        // Pretend at least one op was done in case we are running a benchmark
        // that does not call FinishedSingleOp().
        if (done_ < 1) done_ = 1;

        std::string extra;
        if (bytes_ > 0) {
            // Rate is computed on actual elapsed time, not the sum of per-thread
            // elapsed times.
            double elapsed = (finish_ - start_) * 1e-6;
            char rate[100];
            snprintf(rate, sizeof(rate), "%6.1f MB/s",
                     (bytes_ / 1048576.0) / elapsed);
            extra = rate;
        }
        AppendWithSpace(&extra, message_);

        fprintf(stdout, "%-12s : %11.3f micros/op;%s%s\n",
                name.ToString().c_str(),
                seconds_ * 1e6 / done_,
                (extra.empty() ? "" : " "),
                extra.c_str());
        if (FLAGS_histogram) {
            fprintf(stdout, "Microseconds per op:\n%s\n", hist_.ToString().c_str());
        }
        fflush(stdout);
    }
};

// State shared by all concurrent executions of the same benchmark.
struct SharedState {
    port::Mutex mu;
    port::CondVar cv;
    int total;

    // Each thread goes through the following states:
    //    (1) initializing
    //    (2) waiting for others to be initialized
    //    (3) running
    //    (4) done

    int num_initialized;
    int num_done;
    bool start;

    SharedState() : cv(&mu) {}
};

// Per-thread state for concurrent executions of the same benchmark.
struct ThreadState {
    int tid;             // 0..n-1 when running in n threads
    Random rand;         // Has different seeds for different threads
    Stats stats;
    SharedState *shared;

    ThreadState(int index)
            : tid(index),
              rand(1000 + index) {
    }
};

class Benchmark {
private:
    pmemkv::KVEngine *kv_;
    int num_;
    int value_size_;
    int reads_;

    void PrintHeader() {
        const int kKeySize = 16;
        PrintEnvironment();
        fprintf(stdout, "Path:       %s\n", FLAGS_db);
        fprintf(stdout, "Engine:     %s\n", FLAGS_engine);
        fprintf(stdout, "Keys:       %d bytes each\n", kKeySize);
        fprintf(stdout, "Values:     %d bytes each\n", FLAGS_value_size);
        fprintf(stdout, "Entries:    %d\n", num_);
        fprintf(stdout, "RawSize:    %.1f MB (estimated)\n",
                ((static_cast<int64_t>(kKeySize + FLAGS_value_size) * num_)
                 / 1048576.0));
        PrintWarnings();
        fprintf(stdout, "------------------------------------------------\n");
    }

    void PrintWarnings() {
#if defined(__GNUC__) && !defined(__OPTIMIZE__)
        fprintf(stdout,
                "WARNING: Optimization is disabled: benchmarks unnecessarily slow\n"
        );
#endif
#ifndef NDEBUG
        fprintf(stdout,
                "WARNING: Assertions are enabled; benchmarks unnecessarily slow\n");
#endif
    }

    void PrintEnvironment() {
#if defined(__linux)
        time_t now = time(NULL);
        fprintf(stderr, "Date:       %s", ctime(&now));  // ctime() adds newline

        FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
        if (cpuinfo != NULL) {
            char line[1000];
            int num_cpus = 0;
            std::string cpu_type;
            std::string cache_size;
            while (fgets(line, sizeof(line), cpuinfo) != NULL) {
                const char *sep = strchr(line, ':');
                if (sep == NULL) {
                    continue;
                }
                Slice key = TrimSpace(Slice(line, sep - 1 - line));
                Slice val = TrimSpace(Slice(sep + 1));
                if (key == "model name") {
                    ++num_cpus;
                    cpu_type = val.ToString();
                } else if (key == "cache size") {
                    cache_size = val.ToString();
                }
            }
            fclose(cpuinfo);
            fprintf(stderr, "CPU:        %d * %s\n", num_cpus, cpu_type.c_str());
            fprintf(stderr, "CPUCache:   %s\n", cache_size.c_str());
        }
#endif
    }

public:
    Benchmark()
            :
            kv_(NULL),
            num_(FLAGS_num),
            value_size_(FLAGS_value_size),
            reads_(FLAGS_reads < 0 ? FLAGS_num : FLAGS_reads) {
    }

    ~Benchmark() {
        delete kv_;
    }

    void Run() {
        PrintHeader();

        const char *benchmarks = FLAGS_benchmarks;
        while (benchmarks != NULL) {
            const char *sep = strchr(benchmarks, ',');
            Slice name;
            if (sep == NULL) {
                name = benchmarks;
                benchmarks = NULL;
            } else {
                name = Slice(benchmarks, sep - benchmarks);
                benchmarks = sep + 1;
            }

            // Reset parameters that may be overridden below
            num_ = FLAGS_num;
            reads_ = (FLAGS_reads < 0 ? FLAGS_num : FLAGS_reads);
            value_size_ = FLAGS_value_size;

            void (Benchmark::*method)(ThreadState *) = NULL;
            bool fresh_db = false;
            int num_threads = FLAGS_threads;

            if (name == Slice("fillseq")) {
                fresh_db = true;
                method = &Benchmark::WriteSeq;
            } else if (name == Slice("fillrandom")) {
                fresh_db = true;
                method = &Benchmark::WriteRandom;
            } else if (name == Slice("overwrite")) {
                method = &Benchmark::WriteRandom;
            } else if (name == Slice("readseq")) {
                method = &Benchmark::ReadSeq;
            } else if (name == Slice("readrandom")) {
                method = &Benchmark::ReadRandom;
            } else if (name == Slice("readmissing")) {
                method = &Benchmark::ReadMissing;
            } else if (name == Slice("deleteseq")) {
                method = &Benchmark::DeleteSeq;
            } else if (name == Slice("deleterandom")) {
                method = &Benchmark::DeleteRandom;
            } else {
                if (name != Slice()) {  // No error message for empty name
                    fprintf(stderr, "unknown benchmark '%s'\n", name.ToString().c_str());
                }
            }

            if (fresh_db) {
                if (kv_ != NULL) {
                    delete kv_;
                    kv_ = NULL;
                }
                string db = FLAGS_db;
                if (db.find("/dev/dax") == 0) {
                    fprintf(stdout, "skipped deleting for DAX device\n");
                } else {
                    auto start = g_env->NowMicros();
                    std::remove(FLAGS_db);
                    fprintf(stdout, "%-12s : %11.3f millis/op;\n", "removed", ((g_env->NowMicros() - start) * 1e-3));
                }
            }

            if (kv_ == NULL) {
                Open();
            }

            if (method != NULL) {
                RunBenchmark(num_threads, name, method);
            }
        }
    }

private:
    struct ThreadArg {
        Benchmark *bm;
        SharedState *shared;
        ThreadState *thread;

        void (Benchmark::*method)(ThreadState *);
    };

    static void ThreadBody(void *v) {
        ThreadArg *arg = reinterpret_cast<ThreadArg *>(v);
        SharedState *shared = arg->shared;
        ThreadState *thread = arg->thread;
        {
            MutexLock l(&shared->mu);
            shared->num_initialized++;
            if (shared->num_initialized >= shared->total) {
                shared->cv.SignalAll();
            }
            while (!shared->start) {
                shared->cv.Wait();
            }
        }

        thread->stats.Start();
        (arg->bm->*(arg->method))(thread);
        thread->stats.Stop();

        {
            MutexLock l(&shared->mu);
            shared->num_done++;
            if (shared->num_done >= shared->total) {
                shared->cv.SignalAll();
            }
        }
    }

    void RunBenchmark(int n, Slice name,
                      void (Benchmark::*method)(ThreadState *)) {
        SharedState shared;
        shared.total = n;
        shared.num_initialized = 0;
        shared.num_done = 0;
        shared.start = false;

        ThreadArg *arg = new ThreadArg[n];
        for (int i = 0; i < n; i++) {
            arg[i].bm = this;
            arg[i].method = method;
            arg[i].shared = &shared;
            arg[i].thread = new ThreadState(i);
            arg[i].thread->shared = &shared;
            g_env->StartThread(ThreadBody, &arg[i]);
        }

        shared.mu.Lock();
        while (shared.num_initialized < n) {
            shared.cv.Wait();
        }

        shared.start = true;
        shared.cv.SignalAll();
        while (shared.num_done < n) {
            shared.cv.Wait();
        }
        shared.mu.Unlock();

        for (int i = 1; i < n; i++) {
            arg[0].thread->stats.Merge(arg[i].thread->stats);
        }
        arg[0].thread->stats.Report(name);

        for (int i = 0; i < n; i++) {
            delete arg[i].thread;
        }
        delete[] arg;
    }

    void Open() {
        assert(kv_ == NULL);
        auto start = g_env->NowMicros();
        kv_ = pmemkv::KVEngine::Open(FLAGS_engine, FLAGS_db, ((size_t) 1024 * 1024 * 1024 * FLAGS_db_size_in_gb));
        fprintf(stdout, "%-12s : %11.3f millis/op;\n", "open", ((g_env->NowMicros() - start) * 1e-3));
    }

    void DoWrite(ThreadState *thread, bool seq) {
        if (num_ != FLAGS_num) {
            char msg[100];
            snprintf(msg, sizeof(msg), "(%d ops)", num_);
            thread->stats.AddMessage(msg);
        }

        KVStatus s;
        int64_t bytes = 0;
        for (int i = 0; i < num_; i++) {
            const int k = seq ? i : (thread->rand.Next() % FLAGS_num);
            char key[100];
            snprintf(key, sizeof(key), "%016d", k);
            string value = string();
            value.append(value_size_, 'X');
            s = kv_->Put(key, value);
            bytes += value_size_ + strlen(key);
            thread->stats.FinishedSingleOp();
            if (s != OK) {
                fprintf(stdout, "Out of space at key %i\n", i);
                exit(1);
            }
        }
        thread->stats.AddBytes(bytes);
    }

    void WriteSeq(ThreadState *thread) {
        DoWrite(thread, true);
    }

    void WriteRandom(ThreadState *thread) {
        DoWrite(thread, false);
    }

    void DoRead(ThreadState *thread, bool seq, bool missing) {
        KVStatus s;
        int64_t bytes = 0;
        int found = 0;
        for (int i = 0; i < reads_; i++) {
            const int k = seq ? i : (thread->rand.Next() % FLAGS_num);
            char key[100];
            snprintf(key, sizeof(key), missing ? "%016d!" : "%016d", k);
            string value;
            if (kv_->Get(key, &value) == OK) found++;
            thread->stats.FinishedSingleOp();
            bytes += value.length() + strlen(key);
        }
        thread->stats.AddBytes(bytes);
        char msg[100];
        snprintf(msg, sizeof(msg), "(%d of %d found)", found, reads_);
        thread->stats.AddMessage(msg);
    }

    void ReadSeq(ThreadState *thread) {
        DoRead(thread, true, false);
    }

    void ReadRandom(ThreadState *thread) {
        DoRead(thread, false, false);
    }

    void ReadMissing(ThreadState *thread) {
        DoRead(thread, false, true);
    }

    void DoDelete(ThreadState *thread, bool seq) {
        for (int i = 0; i < num_; i++) {
            const int k = seq ? i : (thread->rand.Next() % FLAGS_num);
            char key[100];
            snprintf(key, sizeof(key), "%016d", k);
            kv_->Remove(key);
            thread->stats.FinishedSingleOp();
        }
    }

    void DeleteSeq(ThreadState *thread) {
        DoDelete(thread, true);
    }

    void DeleteRandom(ThreadState *thread) {
        DoDelete(thread, false);
    }
};

int main(int argc, char **argv) {
    // Print usage statement if necessary
    if (argc != 1) {
        if ((strcmp(argv[1], "?") == 0) || (strcmp(argv[1], "-?") == 0)
            || (strcmp(argv[1], "h") == 0) || (strcmp(argv[1], "-h") == 0)
            || (strcmp(argv[1], "-help") == 0) || (strcmp(argv[1], "--help") == 0)) {
            fprintf(stderr, "%s", USAGE.c_str());
            exit(1);
        }
    }

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        int n;
        char junk;
        if (leveldb::Slice(argv[i]).starts_with("--benchmarks=")) {
            FLAGS_benchmarks = argv[i] + strlen("--benchmarks=");
        } else if (strncmp(argv[i], "--engine=", 9) == 0) {
            FLAGS_engine = argv[i] + 9;
        } else if (sscanf(argv[i], "--histogram=%d%c", &n, &junk) == 1 && (n == 0 || n == 1)) {
            FLAGS_histogram = n;
        } else if (sscanf(argv[i], "--num=%d%c", &n, &junk) == 1) {
            FLAGS_num = n;
        } else if (sscanf(argv[i], "--reads=%d%c", &n, &junk) == 1) {
            FLAGS_reads = n;
        } else if (sscanf(argv[i], "--threads=%d%c", &n, &junk) == 1) {
            FLAGS_threads = n;
        } else if (sscanf(argv[i], "--value_size=%d%c", &n, &junk) == 1) {
            FLAGS_value_size = n;
        } else if (strncmp(argv[i], "--db=", 5) == 0) {
            FLAGS_db = argv[i] + 5;
        } else if (sscanf(argv[i], "--db_size_in_gb=%d%c", &n, &junk) == 1) {
            FLAGS_db_size_in_gb = n;
        } else {
            fprintf(stderr, "Invalid flag '%s'\n", argv[i]);
            exit(1);
        }
    }

    // Choose a location for the test database if none given with --db=<path>
    if (FLAGS_db == NULL) {
        FLAGS_db = "/dev/shm/pmemkv";
    }

    // Run benchmark against default environment
    g_env = leveldb::Env::Default();
    Benchmark benchmark;
    benchmark.Run();
    return 0;
}
