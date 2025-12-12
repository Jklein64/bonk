#include "napi.h"
#include "tet.hpp"
#include <vector>

class BonkWrapper: public Napi::ObjectWrap<BonkWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "BonkInstance", {
      InstanceMethod("loadMesh", &BonkWrapper::loadMesh),
      InstanceMethod("prepareThree", &BonkWrapper::prepareThree),
      InstanceMethod("isThreeReady", &BonkWrapper::isThreeReady),
      InstanceMethod("getIndices", &BonkWrapper::getIndices),
      InstanceMethod("getVertices", &BonkWrapper::getVertices),
      InstanceMethod("initModalContext", &BonkWrapper::initModalContext),
      InstanceMethod("bonk", &BonkWrapper::bonk),
      InstanceMethod("runModal", &BonkWrapper::runModal),
      InstanceMethod("getModalResults", &BonkWrapper::getModalResults)
    });
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);
    exports.Set("BonkInstance", func);
    return exports;
  }
  BonkWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<BonkWrapper>(info) {
    auto env = info.Env();
    actualInstance_ = new BonkInstance();
  }
  ~BonkWrapper() {
    delete actualInstance_;
  }
private:
  BonkInstance* actualInstance_;
  Napi::Value loadMesh(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
      Napi::TypeError::New(env, "loadMesh requires a string argument");
      return env.Null();
    }
    std::string str = info[0].As<Napi::String>().Utf8Value();
    auto res = actualInstance_->loadMesh(str);
    return Napi::Number::New(env, static_cast<int>(res));
  }
  Napi::Value prepareThree(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    auto res = actualInstance_->prepareThree();
    return Napi::Number::New(env, static_cast<int>(res));
  }
  Napi::Value isThreeReady(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    auto res = actualInstance_->threeReady();
    return Napi::Boolean::New(env, res);
  }
  Napi::Value getIndices(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    auto res = actualInstance_->getIndices();
    Napi::Array arr = Napi::Array::New(env, res.size());
    for (size_t i = 0; i < res.size(); i++) {
      arr.Set(i,Napi::Number::New(env, res[i]));
    }
    return arr;
  }
  Napi::Value getVertices(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    auto res = actualInstance_->getVertices();
    Napi::Array arr = Napi::Array::New(env, res.size());
    for (size_t i = 0; i < res.size(); i++) {
      arr.Set(i,Napi::Number::New(env, res[i]));
    }
    return arr;
  }
  Napi::Value initModalContext(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber() || (info.Length() >= 4 && !info[3].IsNumber()) || (info.Length() >= 5 && !info[4].IsNumber())) {
      Napi::TypeError::New(env, "initModalContext requires 3-5 numeric arguments");
      return env.Null();
    }
    double density = info[0].As<Napi::Number>().DoubleValue();
    double k = info[1].As<Napi::Number>().DoubleValue();
    double dt = info[2].As<Napi::Number>().DoubleValue();
    double damping = 0.0; double freqDamping = 0.01;
    if (info.Length() >= 4) {
      damping = info[3].As<Napi::Number>().DoubleValue();
    }
    if (info.Length() >= 5) {
      freqDamping = info[4].As<Napi::Number>().DoubleValue();
    }
    auto res = actualInstance_->initModalContext(density, k, dt, damping, freqDamping);
    return Napi::Number::New(env, static_cast<int>(res));
  }
  Napi::Value bonk(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    if (info.Length() < 3 || !info[0].IsArray() || !info[1].IsArray() || !info[2].IsArray() || info[2].As<Napi::Array>().Length() != 3) {
      Napi::TypeError::New(env, "bonk requires 3 numeric array arguments");
      return env.Null();
    }
    Napi::Array indices = info[0].As<Napi::Array>();
    Napi::Array weights = info[0].As<Napi::Array>();
    Napi::Array force = info[0].As<Napi::Array>();
    std::vector<int> inds {};
    std::vector<double> ws {};
    std::array<double, 3> forceDir {};
    for (size_t i = 0; i < indices.Length(); i++){
      auto v {indices.Get(i)};
      if (!v.IsNumber()) {
        Napi::TypeError::New(env, "invalid element type in index array");
        return env.Null();
      }
      inds.push_back(v.As<Napi::Number>().Int32Value());
    }
    for (size_t i = 0; i < weights.Length(); i++){
      auto v {indices.Get(i)};
      if (!v.IsNumber()) {
        Napi::TypeError::New(env, "invalid element type in index array");
        return env.Null();
      }
      ws.push_back(v.As<Napi::Number>().DoubleValue());
    }
    for (int i = 0; i < 3; i++){
      auto v {indices.Get(i)};
      if (!v.IsNumber()) {
        Napi::TypeError::New(env, "invalid element type in index array");
        return env.Null();
      }
      forceDir[i] = v.As<Napi::Number>().DoubleValue();
    }
    auto res = actualInstance_->bonk(inds, ws, forceDir);
    return Napi::Number::New(env, static_cast<int>(res));
  }
  Napi::Value runModal(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    if (info.Length() == 0 || !info[0].IsNumber()) {
      Napi::TypeError::New(env, "runModal requires a numeric argument");
      return env.Null();
    }
    auto count = info[0].As<Napi::Number>().Int32Value();
    auto res = actualInstance_->runModal(count);
    return Napi::Number::New(env, static_cast<int>(res));
  }
  Napi::Value getModalResults(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    auto res = actualInstance_->getResults();
    Napi::Array arr = Napi::Array::New(env, res.size());
    for (size_t i = 0; i < res.size(); i++) {
      arr.Set(i, Napi::Number::New(env, res[i]));
    }
    return arr;
  }
};

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return BonkWrapper::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitAll)