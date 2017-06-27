/*
 * Copyright 2017, Intel Corporation
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

#include <iostream>
#include <random>
#include <unistd.h>
#include <sys/time.h>
#include "pmemkv.h"

#define LOG(msg) std::cout << msg << "\n"

using namespace pmemkv;

static std::string USAGE = "Usage: pmemkv_stress [r|w] [path] [size in MB]";
static int count = 950000;
static std::string path = "/dev/shm/pmemkv";
static size_t size = ((size_t) 1024 * 1024 * 1000);

static const char* LOREM_IPSUM_50 =
        " Lorem ipsum dolor sit amet, consectetur volutpat.";
static const char* LOREM_IPSUM_100 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras sed turpis a metus suscipit volutpat.";
static const char* LOREM_IPSUM_200 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec facilisis ipsum ipsum, nec sollicitudin nulla pharetra at. Sed accumsan ut felis sed ornare. Aliquam maximus dui congue ipsum cras amet.";
static const char* LOREM_IPSUM_400 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum eu tristique lorem. Integer sodales mi non ullamcorper faucibus. Suspendisse efficitur tortor mi. Phasellus facilisis placerat molestie. Proin tempus mollis volutpat. Quisque gravida hendrerit erat et commodo. Integer eu dolor et velit auctor pharetra non id sapien. Duis consectetur magna ut odio aliquam, vestibulum bibendum sed.";
static const char* LOREM_IPSUM_800 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus bibendum ante sem, vel tincidunt odio aliquet vitae. Vestibulum eget laoreet sem. Donec ultrices scelerisque odio quis pretium. Proin semper in diam nec scelerisque. Fusce id ornare dui. Ut hendrerit interdum cursus. Praesent fringilla non nibh eget rhoncus. In eget est nec enim imperdiet scelerisque. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Donec ac egestas mi. Aenean a urna dui. Suspendisse urna nisl, rhoncus eget risus non, ultricies aliquam ante. Curabitur libero ante, imperdiet a consequat id, cursus eget eros. Suspendisse semper arcu odio, id pellentesque lacus eleifend sed. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Ut metus.";
static const char* LOREM_IPSUM_1200 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas quis elit metus. Cras risus ante, pellentesque eu porta eget, feugiat sit amet ipsum. Sed in libero egestas, vulputate mauris sed, interdum justo. Cras maximus est eu nisl pharetra ullamcorper. In sollicitudin quam sit amet consequat eleifend. Interdum et malesuada fames ac ante ipsum primis in faucibus. Aliquam ut justo odio. Nam condimentum ullamcorper ex sed malesuada. Nulla eu bibendum ex, at luctus lacus. Vivamus in feugiat diam. Nunc dapibus ipsum sit amet purus hendrerit bibendum. Integer tincidunt leo consequat, euismod nunc ut, ultrices lacus. In et justo porta, luctus mi nec, egestas turpis. Etiam cursus nibh ac libero commodo, gravida porttitor mauris consectetur. Suspendisse scelerisque et urna ut varius. Nullam leo sem, interdum non semper vitae, viverra non sem. Nullam commodo dictum sem, eu facilisis justo blandit et. Pellentesque malesuada lacus sit amet turpis pharetra mollis. Etiam sem arcu, malesuada a malesuada varius, consequat ac eros. Integer fermentum, tortor in venenatis tincidunt, odio massa placerat nulla, quis viverra orci massa fringilla enim. Praesent suscipit risus diam, eget cras amet.";
static const char* LOREM_IPSUM_1600 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ut laoreet dolor vel tellus scelerisque auctor. Nam mauris ex, accumsan vel magna vitae, sodales vehicula lacus. Integer tincidunt rhoncus est, sed laoreet velit malesuada vitae. Vivamus vel ante facilisis, pulvinar tortor venenatis, consectetur ex. Etiam congue hendrerit porta. Curabitur porta a lacus eu gravida. Fusce sodales felis ut mauris luctus bibendum. Pellentesque eleifend a augue id iaculis. Maecenas a mi et ex interdum laoreet. Integer sit amet nulla non tellus rhoncus commodo vel ut sem. Aliquam erat volutpat. Aenean vehicula arcu a dui egestas, a dapibus justo imperdiet. Curabitur eu lectus porttitor, sagittis urna et, posuere massa. Nunc dolor est, hendrerit imperdiet leo sit amet, imperdiet pharetra eros. Curabitur rhoncus eleifend risus, ac hendrerit justo venenatis a. Quisque consectetur ligula vestibulum nisl varius, posuere vulputate metus commodo. Mauris vel lorem gravida, sollicitudin enim id, iaculis odio. Proin vulputate arcu at tristique congue. Aliquam cursus et nisl in pharetra. Nulla vulputate, tortor at laoreet feugiat, massa risus pellentesque purus, et faucibus nunc ipsum eu sapien. Donec faucibus augue a quam dapibus faucibus. Mauris iaculis maximus pharetra. Nullam turpis mi, tempor sit amet sem nec, pellentesque tempor nisi. Morbi vulputate condimentum sem non commodo. Duis bibendum, ex vel gravida efficitur, risus orci dignissim nulla, rhoncus sodales ligula augue quis lacus. Phasellus sit amet elit erat. Donec quis augue sit amet lectus condimentum semper. Ut iaculis nulla cras amet.";
static const char* LOREM_IPSUM_3200 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla dignissim dui est, in lobortis felis pulvinar eget. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Vivamus venenatis suscipit sem, vel tristique elit luctus et. Integer eu pellentesque elit, sed iaculis tortor. Morbi et iaculis arcu. Aenean facilisis quam a sem sollicitudin, nec vehicula lorem viverra. Nulla convallis, risus id bibendum semper, lectus sem condimentum erat, in posuere sapien justo vel augue. Ut id pharetra eros. Nulla sit amet eleifend quam, a mollis orci. Curabitur ac rutrum ante. Integer et molestie nunc. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Duis ornare vel nisi ac sodales. Duis sit amet est varius, dapibus eros id, mollis ex. Aliquam ultrices bibendum venenatis. Donec vitae laoreet felis. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Sed gravida faucibus dolor, eget congue orci sodales id. Morbi non est urna. Curabitur et erat in felis tempus aliquet eu convallis orci. Pellentesque condimentum blandit tortor. Cras tincidunt hendrerit mauris sed hendrerit. Quisque eget erat id sapien ultrices eleifend. Praesent dignissim elementum luctus. Maecenas fermentum orci vitae lectus imperdiet gravida. Sed eget tellus sapien. Donec faucibus arcu ligula, ut sagittis urna mollis et. Phasellus cursus nulla a viverra aliquet. Duis tincidunt, tellus et maximus vestibulum, ipsum justo elementum est, ac consectetur neque lacus ac quam. Nunc vestibulum accumsan ligula, et rhoncus mi venenatis vitae. Proin tempus id felis et rutrum. Cras luctus nisl id erat consequat maximus. Morbi fermentum tempus maximus. Sed malesuada varius nunc, fringilla ultrices magna tristique vitae. Integer ac tellus nec lectus pharetra vehicula a at nunc. Curabitur finibus imperdiet enim id feugiat. Proin placerat, nibh et interdum accumsan, risus arcu facilisis erat, nec posuere nisl ligula luctus nisl. Phasellus quis porttitor felis. Praesent efficitur, dolor a venenatis volutpat, ipsum mauris molestie erat, nec semper lacus ex at elit. Sed ut pharetra urna, et sodales dolor. Sed augue urna, finibus ut elit a, auctor mollis tortor. Pellentesque sit amet ipsum non tellus volutpat semper ac eget purus. Morbi fermentum odio vulputate orci semper, a luctus velit venenatis. Maecenas iaculis nulla nunc, a pulvinar libero aliquet vitae. Donec vel ex vitae velit tempor interdum. Suspendisse quis eros convallis, ornare ex et, vestibulum ligula. Nulla nec luctus dui. Cras mollis bibendum dui, porttitor efficitur diam tempor in. Aenean elementum varius tincidunt. Donec in eros eu enim faucibus semper interdum vel turpis. Quisque eleifend ante blandit est egestas viverra. Fusce vel turpis blandit, tempor mi ullamcorper, auctor orci. Integer gravida lobortis leo, sed dignissim odio malesuada id. Nam in felis at erat iaculis placerat. Fusce ac luctus neque, id placerat lacus. Nulla velit nisi, ornare ut quam sed, ultricies fringilla purus. Aliquam erat volutpat. Nunc pretium, purus in facilisis fringilla, ipsum turpis congue libero, eu malesuada urna metus non mauris. Integer cras amet.";
static const char* LOREM_IPSUM_4800 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer varius placerat libero, at pretium felis interdum sed. Pellentesque elit dui, fermentum sed nisl quis, consectetur auctor nulla. Donec volutpat egestas porttitor. Vivamus et nisl purus. Morbi eu felis ut mi congue faucibus eu nec lorem. Maecenas a orci eleifend, rhoncus nibh id, vestibulum tellus. In egestas posuere massa in faucibus. Vivamus eget mollis nisi, sit amet vulputate arcu. Nam molestie posuere sem, non elementum felis semper sed. Quisque sed pulvinar diam. Nunc sit amet orci dui. Pellentesque vel leo at ipsum imperdiet aliquet id a mi. Maecenas eros ante, malesuada at sapien vel, pretium eleifend purus. In urna nibh, varius semper accumsan non, gravida sit amet nunc. Ut odio metus, eleifend vitae ultrices eu, mattis a lorem. Mauris vitae libero a mi commodo egestas et at orci. Curabitur sagittis nibh vitae dolor auctor mollis. Ut nec auctor lacus. Nulla facilisis, enim vitae consequat elementum, ipsum sem sodales magna, aliquet dignissim turpis mi eu quam. Sed sem neque, varius eget pulvinar vel, pharetra quis massa. Praesent in arcu lorem. Aliquam erat volutpat. Vestibulum eros magna, dictum a purus non, pharetra mattis turpis. Donec vel purus ac eros finibus aliquam. Quisque tempor aliquet dolor. Aliquam erat volutpat. Nunc tempor ac nunc nec viverra. Nullam condimentum leo malesuada, iaculis urna ut, imperdiet ante. Vivamus ut massa lectus. Praesent fringilla quis turpis id commodo. Integer laoreet metus non nibh consequat, at dignissim tellus tristique. Etiam pharetra nisl eu velit fringilla congue. Quisque placerat ornare ipsum, eget posuere elit pretium vitae. Donec ac augue id felis consectetur congue sit amet in nulla. Donec posuere quis ante non sollicitudin. Nulla elementum turpis nunc, at bibendum elit imperdiet eu. Nulla erat ex, vestibulum nec eros sit amet, semper viverra turpis. Fusce suscipit lorem lorem, non sodales leo molestie at. Cras nec rutrum dui. In feugiat finibus venenatis. Morbi dignissim, arcu vel porttitor sollicitudin, nibh purus dictum nulla, at mattis mi nisl eget lectus. Nam blandit lectus quis orci venenatis lacinia. Mauris nulla justo, maximus nec neque vitae, congue vehicula sapien. Nulla sem lorem, lacinia ut massa dapibus, ullamcorper hendrerit lacus. Sed a tortor mattis, volutpat nunc non, vulputate mi. Nulla id ullamcorper felis. Fusce sodales placerat dapibus. Aenean eu nisi in elit pellentesque fringilla. Donec nulla orci, accumsan vel elementum eu, vestibulum non tellus. Vestibulum tristique mauris vel lacus gravida efficitur. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Quisque ultricies at lorem ut tincidunt. Mauris vestibulum in turpis et venenatis. Vestibulum at lectus eget magna fermentum sodales. Nam a ornare diam. Integer nec euismod velit. Maecenas sollicitudin neque purus, sed eleifend elit finibus id. Vivamus imperdiet vitae elit at hendrerit. Vestibulum lacus urna, dignissim varius elit quis, accumsan dictum magna. Curabitur faucibus diam orci, in dapibus mauris luctus eget. Quisque consectetur nulla eu diam efficitur volutpat. Fusce gravida vel odio in aliquam. Aliquam a gravida nisi, vel iaculis erat. Pellentesque non tellus vel ligula condimentum dignissim fringilla in turpis. Cras imperdiet lorem eu nisi lacinia tincidunt. Donec in neque at tellus consectetur dapibus. Aenean maximus ipsum a neque posuere, ac laoreet turpis interdum. Mauris egestas risus a scelerisque fringilla. Vivamus id erat at purus dapibus bibendum. Curabitur sit amet varius enim, vitae mattis magna. Integer vel elit sit amet tortor volutpat finibus. Proin gravida libero eget sem sagittis cursus. Quisque non tempus augue. Pellentesque consectetur aliquet neque sit amet faucibus. Vivamus efficitur luctus odio, eget mollis massa interdum quis. Suspendisse id dolor nec mauris scelerisque pellentesque vitae vel justo. Nullam rhoncus turpis ac magna ullamcorper varius. Quisque sit amet sem elit. Cras erat mi, molestie in pellentesque eget, tincidunt vitae tellus. Vivamus id condimentum tellus. Maecenas placerat enim vel fringilla molestie. Vivamus facilisis sapien massa, ut eleifend odio malesuada eu. Praesent vehicula tellus a dapibus convallis. Vestibulum varius ipsum a justo ultrices, ac pellentesque purus posuere. Fusce ut felis in purus efficitur cursus. In placerat diam arcu, et auctor nisi malesuada sed. Vestibulum in felis et est aliquam pulvinar non non turpis. Nulla a facilisis libero, eu ullamcorper diam. Donec tincidunt et ante egestas venenatis. Donec sit amet porttitor ex. Mauris vestibulum nisl eu condimentum placerat. Sed orci arcu, sagittis in ex a, suscipit condimentum purus. Pellentesque vel imperdiet lectus. Quisque augue nunc, pulvinar ut enim posuere.";

static unsigned long current_millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long) (tv.tv_sec) * 1000 + (unsigned long long) (tv.tv_usec) / 1000;
}

static KVTree* open() {
    auto started = current_millis();
    auto kv = new KVTree(path, size);
    LOG("   in " << current_millis() - started << " ms");
    return kv;
}

static string formatKey(int i) {
    string key = std::to_string(i);
    key.append(20 - key.length(), 'X');
    return key;
}

static void testGetRandom(KVTree* kv) {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(1, count - 1);
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        std::string value;
        kv->Get(formatKey(distribution(generator)), &value);
    }
    LOG("   in " << current_millis() - started << " ms");
}

static void testGetSequential(KVTree* kv) {
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        std::string value;
        kv->Get(formatKey(i), &value);
    }
    LOG("   in " << current_millis() - started << " ms");
}

static void testPut(KVTree* kv) {
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        if (kv->Put(formatKey(i), LOREM_IPSUM_800) != OK) {
            std::cout << "Out of space at key " << std::to_string(i) << "\n";
            exit(-42);
        }
    }
    LOG("   in " << current_millis() - started << " ms");
}

static void testRemove(KVTree* kv) {
    auto started = current_millis();
    for (int i = 0; i < count; i++) kv->Remove(formatKey(i));
    LOG("   in " << current_millis() - started << " ms");
}

static void usage_exit(int exit_code) {
    ((exit_code == EXIT_SUCCESS) ? std::cout : std::cerr) << USAGE << "\n";
    exit(exit_code);
}

int main(int argc, char** argv) {
    // assume reading by default when present
    bool reading = (access(path.c_str(), F_OK) == 0);

    // validate command line args if present
    if (argc > 1) {
        std::string command(argv[1]);
        if (command == "h" or command == "-help" or command == "--help")
            usage_exit(EXIT_SUCCESS);
        else if (argc != 4)
            usage_exit(EXIT_FAILURE);
        else if (command == "r" or command == "read")
            reading = true;
        else if (command == "w" or command == "write")
            reading = false;
        else usage_exit(EXIT_FAILURE);
        path = argv[2];
        size = std::stoul(argv[3]) * (1024 * 1024);
        count = (int) (size / 1100);
    }

    // run read or write workload
    if (reading) {
        LOG("\nOpening for reads: " + path);
        KVTree* kv = open();
        for (int i = 0; i < 8; i++) {
            LOG("Getting " << count << " random values");
            testGetRandom(kv);
            LOG("Getting " << count << " sequential values");
            testGetSequential(kv);
        }
        delete kv;
    } else {
        LOG("\nOpening for writes: " + path);
        KVTree* kv = open();
        LOG("Inserting " << count << " values");
        testPut(kv);
//      LOG("Updating " << count << " values");
//      testPut(kv);
//      LOG("Removing " << count << " values");
//      testRemove(kv);
//      LOG("Reinserting " << count << " values");
//      testPut(kv);
        delete kv;
    }
    LOG("\nFinished");
    return 0;
}
