include("build_tools")

group("third_party")
project("elemental-forms")
  uuid("04444ac4-de86-40bc-b6b3-c7684c50cfa9")
  kind("StaticLib")
  language("C++")
  links({})
  defines({
  })
  includedirs({
    ".",
    "src",
  })
  recursive_platform_files("src/")

if os.is("windows") then
  include("testbed")
end
