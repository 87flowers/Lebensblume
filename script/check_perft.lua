local reference_engine = {"/home/lily/repos/stoat/stoat-0.1.19-native"}
local reference_engine_perft_cmd = "splitperft"
local dev_engine = {"zig", "build", "run", "-Doptimize=ReleaseSafe"}
local dev_engine_perft_cmd = "perft"

local posix = require("posix")

function starts_with(haystack, start)
   return string.sub(haystack, 1, string.len(start)) == start
end

function popen2(cmd)
    local r1, w1 = posix.pipe()
    local r2, w2 = posix.pipe()
    assert(r1 ~= nil or r2 ~= nil, "pipe() failed")
    local pid, err = posix.fork()
    assert(pid ~= nil, "fork() failed")
    if pid == 0 then
        posix.close(w1)
        posix.close(r2)
        posix.dup2(r1, posix.fileno(io.stdin))
        posix.dup2(w2, posix.fileno(io.stdout))
        posix.close(r1)
        posix.close(w2)
        local cmd0 = table.remove(cmd, 1)
        local ret, err = posix.execp(cmd0, cmd)
        assert(ret ~= nil, "execp() failed")
        posix._exit(1)
        return
    end
    posix.close(r1)
    posix.close(w2)
    return pid, w1, r2
end

function run(cmd, stdin)
    local pid, sw, sr = popen2(cmd)
    posix.write(sw, stdin)
    local output = ""
    while true do
        output = output .. posix.read(sr, 4096)
        _, status = posix.wait(pid, posix.WNOHANG)
        if status ~= "running" then break end
    end
    output = output .. posix.read(sr, 4096)
    posix.close(sr)
    posix.close(sw)
    return output
end

function runPerft(cmd, perft_cmd, fen, moves, depth)
    if moves ~= nil and moves ~= "" then moves = " moves " .. moves else moves = "" end
    local output = run(cmd, "position sfen " .. fen .. moves .. "\n" .. perft_cmd .. " " .. depth .. "\nquit\n")
    local result = {}
    for line in output:gmatch("[^\r\n]+") do
        local colon = string.find(line, "[\t: ]+")
        if colon ~= nil and not starts_with(line, "info") then
            local move = line:sub(1, colon - 1)
            local count = line:sub(colon + 1):gsub("%s+", "")
            result[move] = count
            if move == "total" then break end
        end
    end
    return result
end

function comparePerft(fen, move_str, depth)
    local s = runPerft(reference_engine, reference_engine_perft_cmd, fen, move_str, depth)
    local b = runPerft(dev_engine, dev_engine_perft_cmd, fen, move_str, depth)

    local moves = {}
    for k, _ in pairs(s) do moves[k] = true end
    for k, _ in pairs(b) do moves[k] = true end

    if move_str == nil then
        print("depth " .. depth .. " for position " .. fen)
    else
        print("drill-into depth " .. depth .. " with moves" .. move_str)
    end

    local mismatch = {}
    for k, _ in pairs(moves) do
        if s[k] ~= b[k] and k ~= "total" then
            table.insert(mismatch, k)
            local sk = s[k]
            local bk = b[k]
            if sk == nil then sk = "0" end
            if bk == nil then bk = "0" end
            print(k .. " ref " .. sk .. " dev " .. bk)
        end
    end
    print("total" .. " ref " .. s["total"] .. " dev " .. b["total"])
    return mismatch
end

function drillIntoPerft(fen, max_depth)
    for depth = 1, max_depth do
        local miss = comparePerft(fen, nil, depth)
        if #miss > 0 then
            local move = ""
            for miss_depth = (depth - 1), 1, -1 do
                print("---")
                move = move .. " " .. miss[1]
                miss = comparePerft(fen, move, miss_depth)
            end
            print(run(reference_engine, "position sfen " .. fen .. " moves " .. move .. "\nd\nquit\n"))
            print("===")
            posix._exit(1)
        end
    end
end

drillIntoPerft("lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1", 7)
drillIntoPerft("l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1", 4)
drillIntoPerft("9/9/9/3k5/9/5K3/9/9/9 b RB2G2S2N2L9Prb2g2s2n2l9p 1", 4)
drillIntoPerft("8l/1l+R2P3/p2pBG1pp/kps1p4/Nn1P2G2/P1P1P2PP/1PS6/1KSG3+r1/LN2+p3L w Sbgn3p 124", 5)
