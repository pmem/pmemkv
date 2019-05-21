#include <libpmemkv.h>
#include <cassert>

using namespace pmemkv;

int main() {
    KVEngine* kv = KVEngine::Start("vsmap", "{\"path\":\"/dev/shm/\"}");

    KVStatus s = kv->Put("key1", "value1");
    assert(s == OK && kv->Count() == 1);

    string value;
    s = kv->Get("key1", &value);
    assert(s == OK && value == "value1");

    kv->Put("key2", "value2");
    kv->Put("key3", "value3");
    kv->All([](const string& k) {});

    s = kv->Remove("key1");
    assert(s == OK && !kv->Exists("key1"));

    delete kv;
    return 0;
}