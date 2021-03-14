#ifndef AFINA_STORAGE_STRIPED_LOCK_LRU_H
#define AFINA_STORAGE_STRIPED_LOCK_LRU_H

#include <map>
#include <string>
#include <vector>
#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

/**
 * # SimpleLRU striped_lock version
 *
 *
 */
class StripedLockLRU : public Afina::Storage {
private:
	StripedLockLRU(size_t shards, size_t max_size) : _shardsn(shards) {
            for (int i = 0; i < _shardsn; ++i){
                _shards.emplace_back( new ThreadSafeSimplLRU(max_size) );
            }
        }
public:
    ~StripedLockLRU() {}

    // use before creating StripedLockLRU object
    static std::unique_ptr<StripedLockLRU> CreateStripedLockLRU(size_t shards = 8, size_t max_size = 64*1024*1024UL) {
	size_t min_shard_size = 1 * 1024 * 1024UL;
	if ( (max_size / shards) < min_shard_size){
	       throw std::runtime_error("Exceeded number of stripes, set min " + std::to_string(max_size / min_shard_size));	
	}
	return std::unique_ptr<StripedLockLRU>(new StripedLockLRU(shards, max_size / shards));
    }

    // see SimpleLRU.h
    bool Put(const std::string &key, const std::string &value) override {
	return _shards[_h(key) % _shardsn]->SimpleLRU::Put(key, value);
    }

    // see SimpleLRU.h
    bool PutIfAbsent(const std::string &key, const std::string &value) override {
	return _shards[_h(key) % _shardsn]->SimpleLRU::PutIfAbsent(key, value);
    }

    // see SimpleLRU.h
    bool Set(const std::string &key, const std::string &value) override {
        return _shards[_h(key) % _shardsn]->SimpleLRU::Set(key, value);
    }

    // see SimpleLRU.h
    bool Delete(const std::string &key) override {
        return _shards[_h(key) % _shardsn]->SimpleLRU::Delete(key);
    }

    // see SimpleLRU.h
    bool Get(const std::string &key, std::string &value) override {
        return _shards[_h(key) % _shardsn]->SimpleLRU::Get(key, value);
    }

private:
    std::vector<std::unique_ptr<ThreadSafeSimplLRU>> _shards;
    std::hash<std::string> _h;
    size_t _shardsn;
};

} // namespace Backend
} // namespace Afina

#endif //AFINA_STORAGE_STRIPED_LOCK_LRU_H
