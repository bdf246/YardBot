#include "C:\Users\admin\Dropbox\autoit-source\CommMG\commMg.au3"
;#include "C:\Users\admin\Dropbox\autoit-source\include\joystic.au3"
#include <GUIConstants.au3>
#include <GUIConstantsEx.au3>
#include <WindowsConstants.au3>
#include <array.au3>
HotKeySet("{Esc}", "Terminate")
_CommSetDllPath(".\CommMG\commg.dll")
$comports=_CommListPorts()
;MsgBox(0,"",$comports)
dim $list
_CommSetport(3, $list, 115200)
$list = _CommPortConnection()
;MsgBox(0,"",$list)
$timeOfLastGoodPacket = TimerInit()
$currentTime = TimerInit()

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
$lblRX=GUICtrlCreateLabel("test",280,40,200)


GUICtrlCreateGroup("Joystick Position",10,100,125,125,1)
GUICtrlCreateLabel('+',10+60,100+60)
$lblCURSOR=GUICtrlCreateLabel('+',10+125,100+125)

GUICtrlCreateGroup("Relay Controls",274,100,216,254)
$btnRelay1=GUICtrlCreateButton("Relay 1",284,115,50)
$btnRelay2=GUICtrlCreateButton("Relay 2",334,115,50)
$btnRelay3=GUICtrlCreateButton("Relay 3",384,115,50)
$btnRelay4=GUICtrlCreateButton("Relay 4",434,115,50)
$btnRelay5=GUICtrlCreateButton("Relay 5",284,135,50)
$btnRelay6=GUICtrlCreateButton("Relay 6",334,135,50)
$btnRelay7=GUICtrlCreateButton("Relay 7",384,135,50)
$btnRelay8=GUICtrlCreateButton("Relay 8",434,135,50)

$btnManual=GUICtrlCreateButton("Manual",434,175,50)
GUISetState(@SW_SHOW)

;_ArrayDisplay(WinGetPos("Remote Control"))
;testmotor()
;DriveMotor(120,60,23)
;exit



while (1)
	$relay=0
	$msg = GUIGetMsg()
	Select
		Case $msg = $gui_event_close
			ExitLoop
		case $msg = $btnRelay1
			$relay=22
		case $msg = $btnRelay2
			$relay=23
		case $msg = $btnRelay3
			$relay=24
		case $msg = $btnRelay4
			$relay=25
		case $msg = $btnRelay5
			$relay=26
		case $msg = $btnRelay6
			$relay=27
		case $msg = $btnRelay7
			$relay=28
		case $msg = $btnRelay8
			$relay=29
		case $msg = $btnManual
			$relay=99
	EndSelect
	$newPOS=MouseGetPos()
	$y=$newPOS[1]-$startPOS[1]
	$x=$newPOS[0]-$startPOS[0]

	if $y>120 then $y=120
	if $y<0 then $y=0
	if $x>120 then $x=120
	if $x<0 then $x=0

	ConsoleWrite(@crlf&'('&$x-125&','&$y-125&','&$x+$y-250&')    ('&$x&','&$y&','&$x+$y&')')
	GUICtrlDelete($lblCURSOR)
	$lblCURSOR=GUICtrlCreateLabel('+',10+$x,100+$y)
	GUISetState(@SW_SHOW)

	DriveMotor($x,$y,$relay)
	getCommand()
	timeout()
WEnd

Func DriveMotor($x = 60, $y = 60, $relay=0)
	$checksum=$x+$y+$relay
	ConsoleWrite($x&','&$y&','&$checksum)
	GUICtrlSetData($lblTX,'('&$x-125&','&$y-125&','&$x+$y-250&')    ('&$x&','&$y&','&$x+$y&')')
	_CommSendByte(Binary(255), 1)
	_CommSendByte(Binary($checksum), 1)
	_CommSendByte(Binary($x), 1)
	_CommSendByte(Binary($y), 1)
	_CommSendByte(Binary($relay), 1)
	_CommSendByte(Binary(254), 1)
	Sleep(1)
EndFunc   ;==>DriveMotor

Func getCommand()
	If _CommGetInputCount() > 0 Then
		$msg = _CommGetstring()
		GUICtrlSetData($lblRX,$msg)
		If StringLeft($msg, 1) Then $currentTime = TimerInit()
		If StringLeft($msg, 1) = 2 Then
			Return 1
		Else
			Return 0
		EndIf
	EndIf
EndFunc   ;==>getCommand

Func Terminate()
	Return
EndFunc   ;==>Terminate

Func timeout()
	Local $fDiff = TimerDiff($currentTime)
	If $fDiff > 3000 Then
		ConsoleWrite(@CRLF & "lost transmition          ");
	EndIf
EndFunc   ;==>timeout

