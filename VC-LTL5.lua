target("VC-LTL-5")
    set_kind("phony")
    before_build("windows", function (target)
        local function find_in_file()
            for _, dir in ipairs(os.dirs("$(projectdir)/*")) do
                local name = dir:match(".*[/\\](.*)")
                if name and name:find("VC%-LTL") then
                    return dir .. [[\build\native\]]
                end
            end
        end
        local function find_in_reg()
            return vformat("$(reg HKEY_CURRENT_USER\Code\VC-LTL;Root)")
        end
        local VC_LTL_Root = find_in_file() or find_in_reg()
        if not VC_LTL_Root or #VC_LTL_Root == 0 then
            cprint("VC-LTL was not found. Install the VC-LTL NuGet package.")
            return
        end
        local WindowsTargetPlatformMinVersion = "5.1.2600.0"
        cprint("VC-LTL Path : %s", VC_LTL_Root)
        cprint("WindowsTargetPlatformMinVersion : %s", WindowsTargetPlatformMinVersion)
        import("core.tool.toolchain")
        local msvc = toolchain.load("msvc")
        local runenvs = msvc:runenvs()
        local includepath = VC_LTL_Root .. [[TargetPlatform\header;]] .. VC_LTL_Root .. [[TargetPlatform\]] .. WindowsTargetPlatformMinVersion .. [[\header;]]
        runenvs.INCLUDE = includepath .. runenvs.INCLUDE
        local arch = target:arch()
        local archpath = (arch == "x86") and "Win32" or "x64"
        cprint("Platform : %s", archpath)
        local libpath = VC_LTL_Root .. [[TargetPlatform\]] .. WindowsTargetPlatformMinVersion .. [[\lib\]] .. archpath .. ";"
        runenvs.LIB = libpath .. runenvs.LIB
    end)
