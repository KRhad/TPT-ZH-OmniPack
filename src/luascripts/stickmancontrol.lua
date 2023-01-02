--[[
Original credit: PhauloRiquelme
https://powdertoy.co.uk/User.html?Name=PhauloRiquelme
https://starcatcher.us/scripts/?view=206

Adapted into Android port by jacob1 (no changes other than to remove broken control tutorial and format with tabs)
]]
local is_mobile = tpt.version.mobilemajor ~= nil
if not is_mobile then
	return
end
local function clamp(number, min, max)
	if number == nil then return 0 end
	local value = number
	if min ~= nil and value < min then
		value = min
	elseif max ~= nil and value > max then
		value = max
	end
	return value
end
elements.property(tpt.el.stkm.id,"Description","Stickman. Don't kill him! Control with Stickman Icon.")
elements.property(tpt.el.stkm.id,"MenuVisible",1)
elements.property(tpt.el.stk2.id,"Description","Second stickman. Don't kill him! Control with Second Stickman Icon.")
elements.property(tpt.el.stk2.id,"MenuVisible",1)
elements.property(tpt.el.figh.id,"MenuVisible",1)
local timer = 0
local stkmbutton = {x = 621, y = 155+4, width = 16, height = 16}
local stk2button = {x = 621, y = 187+4, width = 16, height = 16}
local stk2button_y = stk2button.y
local fighnumber_y_default = 219
local fighnumber_y = fighnumber_y_default
local stkmcontrol = false
local stkmleft = false
local stkmright = false
local stkmup = true
local stkmaction = false
local stk2control = false
local stk2left = false
local stk2right = false
local stk2up = true
local stk2action = false
--[[local stkmrocket = false
local stkmfan = false
local stk2rocket = false
local stk2fan = false]]
local function mouseclick(mousex,mousey,button,event,wheel)
	-- STKM
	if event == 2 then
		stkmcontrol = false
		stkmup = true
		stkmaction = false
		stkmleft = false
		stkmright = false
	end
	if mousex >= stkmbutton.x and mousey >= stkmbutton.y and mousex < (stkmbutton.x+stkmbutton.width) and mousey < (stkmbutton.y+stkmbutton.height) then
		if event == 1 then
			stkmcontrol = true
		end
	end
	if tpt.set_pause() < 1 then
		local number = 0x00
		if event == 3 then
			if stkmcontrol then
				if mousex < stkmbutton.x then
					number = number + 0x01
					stkmleft = true
					stkmright = false
				elseif mousex >= stkmbutton.x+18 then
					number = number + 0x02
					stkmleft = false
					stkmright = true
				else
					stkmleft = false
					stkmright = false
				end

				if mousey < stkmbutton.y then
					number = number+0x04
					stkmup = false
				else
					stkmup = true
				end

				if mousey >= stkmbutton.y+32 then
					number = number + 0x08
					stkmaction = true
				else
					stkmaction = false
				end

			end
		end
		sim.stickman(1,"comm",number)
	end
	-- STK2
	if event == 2 then
		stk2control = false
		stk2up = true
		stk2action = false
		stk2left = false
		stk2right = false
	end
	if mousex >= stk2button.x and mousey >= stk2button.y and mousex < (stk2button.x+stk2button.width) and mousey < (stk2button.y+stk2button.height) then
		if event == 1 then
			stk2control = true
		end
	end
	if tpt.set_pause() < 1 then
		local number = 0x00
		if event == 3 then
			if stk2control then
				if mousex < stk2button.x then
					number = number + 0x01
					stk2left = true
					stk2right = false
				elseif mousex >= stk2button.x+18 then
					number = number + 0x02
					stk2left = false
					stk2right = true
				else
					stk2left = false
					stk2right = false
				end

				if mousey < stk2button.y then
					number = number+0x04
					stk2up = false
				else
					stk2up = true
				end

				if mousey >= stk2button.y+32 then
					number = number + 0x08
					stk2action = true
				else
					stk2action = false
				end

			end
		end
		sim.stickman(2,"comm",number)
	end

end
tpt.register_mouseevent(mouseclick)
local function smallstep()
	local fighnumber = 0
	for stkm = 3,102,1 do
		if sim.stickman(stkm,"spwn") == 1 then
			fighnumber = fighnumber + 1
		end
	end
	local stkmnumber = 0
	for stkm = 1,2,1 do
		if sim.stickman(stkm,"spwn") == 1 then
			stkmnumber = stkmnumber + 1
		end
	end
	local stkmspawned, stk2spawned = sim.stickman(1,"spwn") == 1, sim.stickman(2,"spwn") == 1

	local danger_random = (math.random(0,1))
	-- STKM
	if stkmspawned then
		if stkmcontrol then
			gfx.fillRect(stkmbutton.x, stkmbutton.y, 18, 32,0,0,0)
			gfx.drawRect(stkmbutton.x, stkmbutton.y, 18, 32,200,200,200)
		else
			if fighnumber > 0 and stkmnumber > 0 then
				gfx.fillRect(stkmbutton.x, stkmbutton.y, stkmbutton.width, stkmbutton.height,0,0,0)
				gfx.drawRect(stkmbutton.x, stkmbutton.y, stkmbutton.width, stkmbutton.height,200,200*danger_random,200*danger_random)
			else
				gfx.fillRect(stkmbutton.x, stkmbutton.y, stkmbutton.width, stkmbutton.height,0,0,0)
				gfx.drawRect(stkmbutton.x, stkmbutton.y, stkmbutton.width, stkmbutton.height,200,200,200)
			end
		end
		if stkmcontrol then
			gfx.fillCircle(clamp(tpt.mousex-2,stkmbutton.x, stkmbutton.x+17),clamp(tpt.mousey-2,stkmbutton.y,stkmbutton.y+31),4,4,200,175,150,255)
			gfx.drawCircle(clamp(tpt.mousex-2,stkmbutton.x, stkmbutton.x+17),clamp(tpt.mousey-2,stkmbutton.y,stkmbutton.y+31),4,4,0,0,0,200)
		else
			gfx.drawRect(stkmbutton.x+3, stkmbutton.y+3-(math.abs(math.cos(timer+45)*3)), 5, 5,200,175,150)
			gfx.drawLine(stkmbutton.x+6, stkmbutton.y+3+6-(math.abs(math.cos(timer+45)*3)), (stkmbutton.x+6)+(math.sin(timer+(45/1.5))*4), ((stkmbutton.y+3+6)+3)-(math.cos(timer+45)*2),150/1.5,150/1.5,200/1.5)
			gfx.drawLine(stkmbutton.x+6, stkmbutton.y+3+6-(math.abs(math.cos(timer+45)*3)), (stkmbutton.x+6)+(math.sin(timer+45)*4), ((stkmbutton.y+3+6)+3)+(math.cos(timer+45)*2),255,255,255)
		end
		local text = ""
		if stkmleft then
			text = "Left"
			if not stkmup then
				text = text.." + Jump"
			elseif stkmaction then
				text = text.." + Action"
			end
		end
		if stkmright then
			text = "Right"
			if not stkmup then
				text = text.." + Jump"
			elseif stkmaction then
				text = text.." + Action"
			end
		end
		if (not stkmleft) and (not stkmright) then
			if not stkmup then
				text = "Jump"
			elseif stkmaction then
				text = "Action"
			end
		end
		gfx.drawText(609-(tpt.textwidth(text)), 155+16, text,200,175,150)
	end
	-- STK2
	if stk2spawned then
		if stkmspawned then
			stk2button.y = stk2button_y
		else
			stk2button.y = stkmbutton.y
		end
		if stk2control then
			gfx.fillRect(stk2button.x, stk2button.y, 18, 32,0,0,0)
			gfx.drawRect(stk2button.x, stk2button.y, 18, 32,200,200,200)
		else
			if fighnumber > 0 and stkmnumber > 0 then
				gfx.fillRect(stk2button.x, stk2button.y, stk2button.width, stk2button.height,0,0,0)
				gfx.drawRect(stk2button.x, stk2button.y, stk2button.width, stk2button.height,200,150*danger_random,200*danger_random)
			else
				gfx.fillRect(stk2button.x, stk2button.y, stk2button.width, stk2button.height,0,0,0)
				gfx.drawRect(stk2button.x, stk2button.y, stk2button.width, stk2button.height,150,150,200)
			end
		end
		if stk2control then
			gfx.fillCircle(clamp(tpt.mousex-2,stk2button.x, stk2button.x+17),clamp(tpt.mousey-2,stk2button.y, stk2button.y+31),4,4,128,128,255,255)
			gfx.drawCircle(clamp(tpt.mousex-2,stk2button.x, stk2button.x+17),clamp(tpt.mousey-2,stk2button.y, stk2button.y+31),4,4,0,0,0,200)
		else
			gfx.drawRect(stk2button.x+3, stk2button.y+3-(math.abs(math.cos(timer+45)*3)), 5, 5,200,175,150)
			gfx.drawLine(stk2button.x+6, stk2button.y+3+6-(math.abs(math.cos(timer+45)*3)), (stk2button.x+6)+(math.sin(timer+(45/1.5))*4), ((stk2button.y+3+6)+3)-(math.cos(timer+45)*2),128/2,128/2,255/2)
			gfx.drawLine(stk2button.x+6, stk2button.y+3+6-(math.abs(math.cos(timer+45)*3)), (stk2button.x+6)+(math.sin(timer+45)*4), ((stk2button.y+3+6)+3)+(math.cos(timer+45)*2),128,128,255)
		end
		local text = ""
		if stk2left then
			text = "Left"
			if not stk2up then
				text = text.." + Jump"
			elseif stk2action then
				text = text.." + Action"
			end
		end
		if stk2right then
			text = "Right"
			if not stk2up then
				text = text.." + Jump"
			elseif stk2action then
				text = text.." + Action"
			end
		end
		if (not stk2left) and (not stk2right) then
			if not stk2up then
				text = "Jump"
			elseif stk2action then
				text = "Action"
			end
		end
		gfx.drawText(609-(tpt.textwidth(text)), 187+8, text,128,128,255)
	end
	-- Number of FIGHs
	if fighnumber > 0 then
		if stkmnumber >= 2 then
			if stk2control then
				fighnumber_y = fighnumber_y_default+8
			else
				fighnumber_y = fighnumber_y_default
			end
		elseif stkmnumber >= 1 then
			if stk2control then
				fighnumber_y = stk2button_y+8
			else
				fighnumber_y = stk2button_y
			end
		else
			fighnumber_y = stkmbutton.y
		end
		gfx.drawCircle(621+5, fighnumber_y+5-(math.abs(math.cos(timer+45)*3)), 2, 2,200,175,150)
		gfx.drawLine(621+6, fighnumber_y+3+6-(math.abs(math.cos(timer+45)*3)), (621+6)+(math.sin(timer+(45/1.5))*4), ((fighnumber_y+3+6)+3)-(math.cos(timer+45)*2),150/1.5,150/1.5,200/1.5)
		gfx.drawLine(621+6, fighnumber_y+3+6-(math.abs(math.cos(timer+45)*3)), (621+6)+(math.sin(timer+45)*4), ((fighnumber_y+3+6)+3)+(math.cos(timer+45)*2),255,255,255)
		gfx.drawText((621+3)-(tpt.textwidth(fighnumber)/2), fighnumber_y+16, fighnumber,200,175,150)
	end
	-- Timer
	if timer >= (math.huge) then
		timer = 0
	end
	timer = timer+0.25
end
tpt.register_step(smallstep)
