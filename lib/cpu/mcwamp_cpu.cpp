//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstdlib>
#include <cassert>
#include <iostream>
#include <map>
#include <vector>

#include <amp_runtime.h>

extern "C" void PushArgImpl(void *ker, int idx, size_t sz, const void *v) {}

namespace Concurrency {

class CPUFallbackView final : public AMPView
{
    std::map<void*, void*> addrs;
public:
    CPUFallbackView(AMPDevice* pMan) : AMPView(pMan) {}
    void Push(void *kernel, int idx, void*& data, void* device, bool isConst) override {
      auto it = addrs.find(data);
      bool find = it != std::end(addrs);
      if (!kernel && !find) {
          addrs[device] = data;
          data = device;
      } else if (kernel && find) {
          data = it->second;
          addrs.erase(it);
      }
  }
};

class CPUFallbackDevice final : public AMPDevice
{
public:
    CPUFallbackDevice() : AMPDevice() { cpu_type = access_type_read_write; }

    std::wstring get_path() const override { return L"fallback"; }
    std::wstring get_description() const override { return L"CPU Fallback"; }
    size_t get_mem() const override { return 0; }
    bool is_double() const override { return true; }
    bool is_lim_double() const override { return true; }
    bool is_unified() const override { return true; }
    bool is_emulated() const override { return true; }

    void* create(size_t count) override { return aligned_alloc(0x1000, count); }
    void release(void *data) override { ::operator delete(data); }
    std::shared_ptr<AMPView> createAloc() override {
        return std::shared_ptr<AMPView>(new CPUFallbackView(this));
    }
};

class CPUContext final : public AMPContext
{
public:
    CPUContext() { Devices.push_back(new CPUFallbackDevice); }
};


static CPUContext ctx;

} // namespace Concurrency

extern "C" void *GetContextImpl() {
  return &Concurrency::ctx;
}
