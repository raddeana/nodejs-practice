global.process = process;
global.Buffer = NativeModule.require("buffer").Buffer;

process.domain = null;
process._exiting = false;

const bindingObj = Object.create(null);
const getBinding = process.binding;

process.binding = function (module) {
  module = String(module);
  let mod = bindingObj[module];

  if (typeof mod !== "object") {
    mod = bindingObj[module] = getBinding(module);
    moduleLoadList.push(`Binding ${module}`);
  }

  return mod;
};