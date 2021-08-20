;; Copyright 1995 Crack dot Com,  All Rights reserved
;; See licensing information for more details on usage rights

;; This file has been heavily rewritten to support the new binding system that
;; allows much more customization and the usage of far more keys.
;;(setf player1 'joystick)

;; DEFAULT BINDINGS

; First up, set our callbacks - they mostly set variables that are checked
; every frame
(setq player-movement 0)
(setq player-jumping 0)
(setq player-activating-obj 0)
(setq player-firing 0)
(setq player-using-special 0)
(setq player-switch-weapon 0)

; The callbacks themselves
(defun player-move-left (keydown) (setq player-movement (if keydown -1 0)))
(defun player-move-right (keydown) (setq player-movement (if keydown 1 0)))
(defun player-jump (keydown) (setq player-jumping (if keydown 1 0)))
(defun player-use (keydown) (setq player-activating-obj (if keydown 1 0)))
(defun player-special (keydown) (setq player-using-special (if keydown 1 0)))
; Player switch weapon is reset to 0 whenever it's processed - this allows these
; to be properly "event based"
(defun player-fire-weapon (keydown) (if keydown (setq player-firing 1)))
(defun player-next-weapon (keydown) (if keydown (setq player-switch-weapon 1)))
(defun player-prev-weapon (keydown) (if keydown (setq player-switch-weapon -1)))

; Register them with the events system
(add_control "left" player-move-left)
(add_control "right" player-move-right)
(add_control "jump" player-jump)
(add_control "use" player-use)
(add_control "fire" player-fire-weapon)
(add_control "special" player-special)
(add_control "next_weapon" player-next-weapon)
(add_control "prev_weapon" player-prev-weapon)

; Now the actual default bindings - these can also be configured via the
; configuration system
; (bind_control) takes a list (or if not a list, a single string) of controls
; and then a name to bind them to as defined by add_control
;
; Valid input types are:
;   - a number: a keyboard scancode (see http://wiki.libsdl.org/SDLScancodeLookup)
;   - a string: a keyboard key name (see http://wiki.libsdl.org/SDL_Scancode)
;               OR "scancode (number)" to define a scancode as if it were a number
;               OR "mouse button (left|middle|right|number)" for a mouse button
;               OR "mouse wheel (up|left|right|down)" for mouse wheel scrolling
;               OR "controller (button)" for a controller button
; Bind arrow keys and the WASD keys (via scancode) to movement/jump
(bind_control ("left" 4) "left")
(bind_control ("right" 7) "right")
(bind_control ("up" 26 44) "jump")
(bind_control ("down" 22) "use")
(bind_control ("right ctrl" 20 "mouse wheel up" "mouse wheel left") "prev_weapon")
(bind_control ("insert" 8 "mouse wheel down" "mouse wheel right") "next_weapon")

(bind_control "mouse button left" "fire")
(bind_control "mouse button right" "special")

(bind_control "controller A" "jump")
(bind_control "controller X" "special")
(bind_control "controller B" "use")
(bind_control "controller LB" "prev_weapon")
(bind_control "controller RB" "next_weapon")
;(bind_controller_axis_as_button "left" "x" 0.25 'player-move-left 'player-move-right)

;; note : this function is called by the game when it collects input from
;; the local player (the machine you are playing on in net games)
;; because it is only executed locally it should not change any global varible
;; Also the rest of the game should not use any input gathering routines such as
;; joy_stat & key_presed.

;;  The returned input from this routine is passed on to the server where it is
;; distributed to other clients.
;; The format for return is '(xv yv B1 B2 B3 mx my)

;; xv = movement left right (-1=left, 0=none, 1=right)
;; yv = movement up and down (-1=up,  0=none, 1=right)
;; B1 = using special power (T or nil)
;; B2 = firing button (T or nil)
;; B3 = toggle to weapons (-1 toggle weapons to the left, 0=no toggle, 1 toggle to the right)
;; mx = mouse x position on the screen
;; my = mouse y position on the screen

;; other possible key names for get_key_code include :
;;   "Up_Arrow","Down_Arrow","Left_Arrow","Right_Arrow",
;;   "Left_Ctrl","Right_Ctrl","Left_Alt","Right_Alt",
;;   "Left_Shift","Right_Shift","Caps_Lock","Num_Lock",
;;   "Home","End","Del","F1","F2","F3","F4","F5","F6",
;;   "F7","F8","F9","F10","Insert"
;;   "a", "b", "c", ...
;; though not all of these keys will work on all operating systems


;; note : at this current time weapon toggle input is ignored and
;; CTRL & INS are used... sorry :)  These have to be event based,
;; not polled so no key pressed are missed.

(setq left-key          (get_key_code "left_arrow"))
(setq right-key         (get_key_code "right_arrow"))
(setq up-key            (get_key_code "up_arrow"))
(setq down-key          (get_key_code "down_arrow"))
(setq weapon-left-key   (get_key_code "right_ctrl"))
(setq weapon-right-key  (get_key_code "insert"))

(setq key-shiftr        (get_key_code "right_shift"))
(setq key-end           (get_key_code "end"))

(setq key-a  (get_key_code "a"))
(setq key-b  (get_key_code "b"))
(setq key-c  (get_key_code "c"))
(setq key-d  (get_key_code "d"))
(setq key-e  (get_key_code "e"))
(setq key-f  (get_key_code "f"))
(setq key-g  (get_key_code "g"))
(setq key-h  (get_key_code "h"))
(setq key-i  (get_key_code "i"))
(setq key-j  (get_key_code "j"))
(setq key-k  (get_key_code "k"))
(setq key-l  (get_key_code "l"))
(setq key-m  (get_key_code "m"))
(setq key-n  (get_key_code "n"))
(setq key-o  (get_key_code "o"))
(setq key-p  (get_key_code "p"))
(setq key-q  (get_key_code "q"))
(setq key-r  (get_key_code "r"))
(setq key-s  (get_key_code "s"))
(setq key-t  (get_key_code "t"))
(setq key-u  (get_key_code "u"))
(setq key-v  (get_key_code "v"))
(setq key-w  (get_key_code "w"))
(setq key-x  (get_key_code "x"))
(setq key-y  (get_key_code "y"))
(setq key-z  (get_key_code "z"))

(setq dray_has_fired 1)
(setq godmode 0)
(setq enemytarget 0)
(setq noclip 0)

(defun get_local_input ()
  ;; fRaBs Twist extension input logic
  (if (eq godmode 1)
      (progn
        (with_object (bg) (set_hp 100))
        (if (or (eq (with_object (bg) (state)) flinch_up)
                (eq (with_object (bg) (state)) flinch_down))
            (with_object (bg) (set_state stopped)))))
  (if (eq noclip 1)
      (progn
        (with_object (bg) (set_gravity 0))
        (if (local_key_pressed up-key)
            (with_object (bg) (set_y (- (y) run_top_speed))))
        (if (local_key_pressed down-key)
            (with_object (bg) (set_y (+ (y) run_top_speed))))
        (if (local_key_pressed left-key)
            (with_object (bg) (set_x (- (x) run_top_speed))))
        (if (local_key_pressed right-key)
            (with_object (bg) (set_x (+ (x) run_top_speed))))
        (with_object (bg) (set_gravity 0))))
  (if (and (eq (mod (game_tick) 15) 0)
           (eq dray_has_fired 1))
      (setq dray_has_fired 0))
  (if (local_key_pressed key-shiftr)
      (save_game (concatenate 'string "save"
                              (digstr (get_save_slot) 4) ".spe")))
  (if (and (eq (third (mouse_stat)) 1)
           (with_object (bg) (eq (current_weapon_type) 7)))
      (if (and (eq dray_has_fired 0)
               (<= (ammo_total 7) 1)
               (not (or (<= (with_object (bg) (hp)) 0)
                        (eq (with_object (bg) (state)) dieing)
                        (eq (with_object (bg) (state)) dead)
                        (eq (with_object (bg) (state)) flinch_up)
                        (eq (with_object (bg) (state)) flinch_down)
                        (eq (with_object (bg) (state)) climbing)
                        (eq (with_object (bg) (state)) climb_off)
                        (eq (with_object (bg) (state)) climb_on))))
          (progn
            (setq dray_has_fired 1)
            (play_sound DEATH_RAY_SND 127 (x) (y))
            (with_object (bg) (add_object DEATH_RAY (x) (- (y)
                                          (/ (picture_height) 2))))
            (with_object (bg) (add_ammo 7 -1)))))
  ;; Original Abuse logic
  (let ((mstat (mouse_stat)))
    (list (if (local_key_pressed left-key) -1
	      (if (local_key_pressed right-key) 1 0))  ;; xv
	  (if (local_key_pressed up-key) -1
	      (if (local_key_pressed down-key) 1 0))  ;; yv
	  (eq (fourth mstat) 1) ;; special button
	  (eq (third mstat) 1) ;; fire button

	  (if (or (eq (fifth mstat) 1)
		  (local_key_pressed weapon-left-key)) -1 ;; weapon toggle
              (if (local_key_pressed weapon-right-key) 1 0))
	  (first mstat) ;; mx
	  (second mstat) ;; my
	  )))

;; XXX: Mac Abuse uses these hardcoded values
;(setq up_key 256)
;(setq left_key 258)
;(setq right_key 259)
;(setq down_key 257)
;(setq special_key 32)
;(setq weapon_left_key 49)
;(setq weapon_right_key 50)
