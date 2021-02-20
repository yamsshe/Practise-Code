--[[
Author: your name
Date: 2020-11-12 14:47:30
LastEditTime: 2020-11-13 15:05:03
LastEditors: Please set LastEditors
Description: In User Settings Edit
FilePath: \code\lua\test1.lua
--]]
--[[
    实现一个函数 凑14 ：输入很多个整数（1<=数值<=13）
    任意两个数相加等于14就可以从数组中删除这两个数，
    求剩余数（从小到大排列）；比如输入{9,1,9,7,5,13 } 输出 {7,9}
--]]

local function func_1(tab)
    local del_idxs = {}
    table.sort(tab)
    for idx, num in ipairs(tab) do
        for i = idx + 1, #tab do
            if (num + tonumber(tab[i]) == 14 and del_idxs[idx] == nil and nil == del_idxs[i]) then
                del_idxs[idx] = idx
                del_idxs[i] = i
                break
            end
        end
    end
    for _, i in pairs(del_idxs) do
        tab[i] = nil
    end
end

local function func_2(tab)
    local low = 1
    local high = #tab
    table.sort(tab)
    while (low < high) do
        local num =  tab[low] + tab[high]
        if (num == 14) then
            table.remove(tab, high)
            table.remove(tab, low)
            low = 1
            high = #tab
        elseif (num < 14) then
            low = low + 1
        elseif (num > 14) then
            high = high - 1
        end
    end
end

local function func_3(tab, markvalue)
    local nums = {}
    for i = 1, markvalue do nums[i] = 0 end 
    -- 校验数据
    for _, v in ipairs(tab) do
        if v < 1 or v >= markvalue then
            return
        end
        nums[v] = nums[v] + 1
    end
    for i = 1, math.floor((markvalue - 1)/2) do
        if nums[i] >= nums[markvalue-i] then
            nums[i] = nums[i] - nums[markvalue - i]
            nums[markvalue -i] = 0
        else
            nums[markvalue - i] = nums[markvalue - i] - nums[i]
            nums[i] = 0
        end
    end
    if (markvalue % 2 == 0) then
        nums[markvalue/2] = nums[markvalue/2] % 2
    end
    for k, v in ipairs(nums) do
        if v > 0 then 
            for times = 1, v do print(k) end
        end
    end
end

--local tab = {9,1,9,7,5,13}
local tab = {9,7,2,4,8,1,9,6,7,7,5,13,1,5,2,6,7,8,9,10,11,12,4,5,6}
func_3(tab, 14)

-- for _, v in pairs(tab) do
--     print(v)
-- end

