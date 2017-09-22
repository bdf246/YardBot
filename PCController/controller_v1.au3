#include ".\CommMG\commMg.au3"
#include ".\joystic.au3"
#include <GUIConstants.au3>
#include <GUIConstantsEx.au3>
#include <WindowsConstants.au3>
#include <array.au3>
#include <ColorConstants.au3>

HotKeySet("{Esc}", "Terminate")
_CommSetDllPath(".\CommMG\commg.dll")
$comports=_CommListPorts()
if $comports='' then
	MsgBox(0,"Error","Xbee not detected",3)
	Exit
Else
	MsgBox(0,"",$comports)
endif
dim $list, $val[1], $ArduinoData=0, $btnRelays[8]
dim $labels_text[8]=['X', 'Y', 'Z', 'R', 'U', 'V', 'POV', 'Buttons']
dim $labels_no=UBound($labels_text)
dim $labels[$labels_no]
dim $labels_value[$labels_no]
$joy    = _JoyInit()
_CommSetport(5, $list, 115200)
$list = _CommPortConnection()
$timeOfLastGoodPacket = TimerInit()
$currentTime = TimerInit()

$label_len=0
for $text in $labels_text
    $len=stringlen($text)
    if $len>$label_len then
        $label_len=$len
    endif
next
$label_len*=6

mousemove(10+60+526,100+60+185,0)
sleep(1000)
dim $startPOS=MouseGetPos()

$parent=GUICreate("Remote Control",500,500)
GUICtrlCreateGroup("Communication",10,10,480,85)
GUICtrlCreateLabel("COM Port:",20,30,50)
$cmbCOM=GUICtrlCreateCombo("",90,25,110)
GUICtrlSetData($cmbCOM,$comports,"com3")
GUICtrlCreateLabel("BAUD Rate:",20,50,60)
$cmbBAUD=GUICtrlCreateCombo("",90,45,110)
GUICtrlCreateLabel("BAUD Rate:",20,50,60)
GUICtrlSetData($cmbBAUD,"9600|14400|38400|57600|115200","9600")
$btnSUBMIT=GUICtrlCreateButton("Submit",20,70,90,20)
$btnREFRESH=GUICtrlCreateButton("Refresh",110,70,90,20)
GUICtrlCreateLabel("Transmit (TX):", 210,20)
$lblTX=GUICtrlCreateLabel("test",280,20,200)
GUICtrlCreateLabel("Receive (RX):", 210,40)
$lblRX=GUICtrlCreateEdit("test",280,40,200,50,$ES_AUTOVSCROLL )


GUICtrlCreateGroup("Joystick Position",10,100,125,125,1)
GUICtrlCreateLabel('+',10+60,100+60)
$lblCURSOR=GUICtrlCreateLabel('+',10+125,100+125)

GUICtrlCreateGroup("Joystick Vaules",140,100,125,125,1)
for $i=0 to $labels_no-1
    GuiCtrlCreatelabel($labels_text[$i]&':', 145, 120+$i*12, $label_len, 12)
    $labels[$i]=GuiCtrlCreatelabel('', 145+$label_len, 120+$i*12, 70, 12)
    $labels_value[$i]=''
next

GUICtrlCreateGroup("Relay Controls",274,100,216,254)
$btnRelays[0]=GUICtrlCreateButton("Relay 1",284,115,50)
$btnRelays[1]=GUICtrlCreateButton("Relay 2",334,115,50)
$btnRelays[2]=GUICtrlCreateButton("Relay 3",384,115,50)
$btnRelays[3]=GUICtrlCreateButton("Relay 4",434,115,50)
$btnRelays[4]=GUICtrlCreateButton("Relay 5",284,135,50)
$btnRelays[5]=GUICtrlCreateButton("Relay 6",334,135,50)
$btnRelays[6]=GUICtrlCreateButton("Relay 7",384,135,50)
$btnRelays[7]=GUICtrlCreateButton("Relay 8",434,135,50)

$btnManual=GUICtrlCreateButton("Manual",434,175,50)
GUISetState(@SW_SHOW)

;_ArrayDisplay(WinGetPos("Remote Control"))
;testmotor()
;DriveMotor(120,60,23)
;exit



while (1)
	$relay=0
	$msg = GUIGetMsg()
	 $coord=_GetJoy($joy,0)
	 ;_ArrayDisplay($coord)
    for $i=0 to UBound($coord)-1
        if $coord[$i]<>$labels_value[$i] then
            GUICtrlSetData($labels[$i], $coord[$i])
            $labels_value[$i]=$coord[$i]
        endif
    next
	Select
		Case $msg = $gui_event_close
			ExitLoop
	EndSelect
	for $i=0 to UBound($btnRelays)-1
		if $msg = $btnRelays[$i] or 2^$i = $coord[7] then
			$relay = $i + 22
			sleep(100)
		endif
	next
    if $msg = $btnManual or $coord[7]=256 then $relay=99
	$newPOS=MouseGetPos()
	$y=$newPOS[1]-$startPOS[1]
	$x=$newPOS[0]-$startPOS[0]
	;$y=int(Round(120*$coord[1]/65535,0))
	;$x=int(Round(120*$coord[0]/65535,0))

	;MsgBox(0,"",$y & "    " & $x)

	if $y>120 then $y=120
	if $y<0 then $y=0
	if $x>120 then $x=120
	if $x<0 then $x=0

	;ConsoleWrite(@crlf&'('&$x-120&','&$y-120&','&$x+$y-240&')    ('&$x&','&$y&','&$x+$y&')')
	GUICtrlDelete($lblCURSOR)
	$lblCURSOR=GUICtrlCreateLabel('+',10+$x,100+$y)
	GUISetState(@SW_SHOW)

	DriveMotor($x,$y,$relay)
	getCommand()
	ParseData()
	timeout()
WEnd

Func DriveMotor($x = 60, $y = 60, $relay=0)
	$checksum=$x+$y+$relay
	;ConsoleWrite($x&','&$y&','&$checksum)
	GUICtrlSetData($lblTX,'('&$x-120&','&$y-120&','&$x+$y-240&')    ('&$x&','&$y&','&$x+$y&')')
	_CommSendByte(Binary(255), 1)
	_CommSendByte(Binary($checksum), 1)
	_CommSendByte(Binary($x), 1)
	_CommSendByte(Binary($y), 1)
	_CommSendByte(Binary($relay), 1)
	_CommSendByte(Binary(254), 1)
	Sleep(1)
EndFunc   ;==>DriveMotor

Func getCommand()
	local $recvArduino=0
	$incoming =_CommGetInputCount()
	while ($incoming > 0 and $ArduinoData=0)
		$rb=_CommReadByte()
		if $rb <> "" then
			GUICtrlSetData($lblRX,$rb&" ",1)
			ConsoleWrite(@crlf&$rb)
		EndIf
		if $recvArduino Then
			if $rb <> 254 Then
				_ArrayAdd($val,$rb)
			Else
				$ArduinoData=1
				$recvArduino=0
				ExitLoop
			EndIf
		ElseIf $rb=255 Then
			$recvArduino=1
		EndIf
	WEnd
EndFunc   ;==>getCommand

func ParseData()
	if $ArduinoData Then
		if $val[1] = 100 then $currentTime = TimerInit()
		if $val[1] = 253 Then
			for $i=0 to UBound($btnRelays)-1
				if $val[$i+2]='1' then
					GUICtrlSetColor($btnRelays[$i], $COLOR_RED)
				Else
					GUICtrlSetColor($btnRelays[$i], $COLOR_GRAY)
				EndIf
			Next
		EndIf
		$ArduinoData=0
		ReDim $val[1]
	EndIf
EndFunc

Func Terminate()
	Return
EndFunc   ;==>Terminate

Func timeout()
	Local $fDiff = TimerDiff($currentTime)
	If $fDiff > 3000 Then
		GUICtrlSetData($lblRX,"Connection lost... "&@crlf,1)
	EndIf
EndFunc   ;==>timeout

