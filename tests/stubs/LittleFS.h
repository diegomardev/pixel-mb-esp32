#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
struct FakeFileState { std::vector<uint8_t> data; size_t offset = 0; };
class File {
  std::shared_ptr<FakeFileState> state;
 public:
  File() = default;
  explicit File(const std::vector<uint8_t>& bytes):state(std::make_shared<FakeFileState>()) { state->data = bytes; }
  explicit operator bool() const { return bool(state); }
  size_t size() const { return state ? state->data.size() : 0; }
  bool seek(size_t offset) { if (!state || offset > state->data.size()) return false; state->offset = offset; return true; }
  int read(uint8_t *target, size_t count) { if (!state) return -1; count = std::min(count, size() - state->offset); std::memcpy(target, state->data.data() + state->offset, count); state->offset += count; return static_cast<int>(count); }
  void close() { state.reset(); }
};
struct FakeFS {
  std::map<std::string,std::vector<uint8_t>> files;
  File open(const std::string &path, const char *) { auto it=files.find(path); return it==files.end()?File():File(it->second); }
};
extern FakeFS LittleFS;
