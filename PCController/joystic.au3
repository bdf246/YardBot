;____________________________________________________________________
;       Original program by Ejoc                                    ;
;       Improved by Adam1213 (autoit 3.2 compatiblity + improved labels         ;
;____________________________________________________________________

#include <GUIConstants.au3>



;======================================
;   _JoyInit()
;======================================
Func _JoyInit()
    Local $joy
    Global $JOYINFOEX_struct    = "dword[13]"

    $joy=DllStructCreate($JOYINFOEX_struct)
    if @error Then Return 0
    DllStructSetData($joy, 1, DllStructGetSize($joy), 1);dwSize = sizeof(struct)
    DllStructSetData($joy, 1, 255, 2)              ;dwFlags = GetAll
    return $joy
EndFunc

;======================================
;   _GetJoy($lpJoy,$iJoy)
;   $lpJoy  Return from _JoyInit()
;   $iJoy   Joystick # 0-15
;   Return  Array containing X-Pos, Y-Pos, Z-Pos, R-Pos, U-Pos, V-Pos,POV
;           Buttons down
;
;           *POV This is a digital game pad, not analog joystick
;           65535   = Not pressed
;           0       = U
;           4500    = UR
;           9000    = R
;           Goes around clockwise increasing 4500 for each position
;======================================
Func _GetJoy($lpJoy,$iJoy)
    Local $coor,$ret

    Dim $coor[8]
    DllCall("Winmm.dll","int","joyGetPosEx", _
            "int",$iJoy, _
            "ptr",DllStructGetPtr($lpJoy))

    if Not @error Then
        $coor[0]    = DllStructGetData($lpJoy,1,3)
        $coor[1]    = DllStructGetData($lpJoy,1,4)
        $coor[2]    = DllStructGetData($lpJoy,1,5)
        $coor[3]    = DllStructGetData($lpJoy,1,6)
        $coor[4]    = DllStructGetData($lpJoy,1,7)
        $coor[5]    = DllStructGetData($lpJoy,1,8)
        $coor[6]    = DllStructGetData($lpJoy,1,11)
        $coor[7]    = DllStructGetData($lpJoy,1,9)
    EndIf

    return $coor
EndFunc