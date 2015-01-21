struct Player {
};
struct Actor {
};

float run_speed = 50000.0;
float jump_speed = 1000.0;
Player *player = 0;

struct Vector3 {
    float x, y, z;
};

DECLARE_HOOK_THISCALL("gamelogic.dll", 0x51680, int, Player_CanJump, Player*)
    return 1;
END_HOOK

DECLARE_HOOK_THISCALL("gamelogic.dll", 0x4ffa0, float, Player_GetJumpSpeed, Player*)
    return jump_speed;
END_HOOK

DECLARE_HOOK_THISCALL("gamelogic.dll", 0x4ff90, float, Player_GetWalkingSpeed, Player*)
    player = this_;
    return run_speed;
END_HOOK

Actor* actor_from_player(Player* p) {
	return (Actor*)((unsigned char*)p - 0x70);
}

DECLARE_HOOK_THISCALL("gamelogic.dll", 0x16f0, void, Actor_GetPosition, Actor*, Vector3* pos)
    CALL_ORIG_VOID(Actor_GetPosition, this_, pos);
END_HOOK

DECLARE_HOOK_THISCALL("gamelogic.dll", 0x1c80, void, Actor_SetPosition, Actor*, Vector3* pos)
    CALL_ORIG_VOID(Actor_SetPosition, this_, pos);
END_HOOK

const char * next_travel_dest = NULL;
DECLARE_HOOK_THISCALL("gamelogic.dll", 0x55ae0, void, Player_FastTravel, Player*, const char *from, const char *to)
	if (next_travel_dest) {
		to = next_travel_dest;
		next_travel_dest = NULL;
	}
	send_message_f("[*] Travelling from %s to %s", from, to);
    CALL_ORIG_VOID(Player_FastTravel, this_, from, to);
END_HOOK

Hook* hooks[] = {
    &Player_CanJump_hook,
    &Player_GetJumpSpeed_hook,
    &Player_GetWalkingSpeed_hook,
	&Actor_GetPosition_hook,
	&Actor_SetPosition_hook,
	&Player_FastTravel_hook,
};

HANDLE freeze_thread = NULL;
Vector3 freeze_pos;
int freeze_freq;
DWORD freeze_loop(void *_) {
	for (;;) {
		CALL_ORIG_VOID(Actor_SetPosition, actor_from_player(player), &freeze_pos);
		Sleep(freeze_freq);
	}
}

void handle_user_command(string cmd, vector<string> args) {
    if (cmd == "rs") {
        if (args.size() > 1) {
            send_message_f("[!] Usage: rs <run_speed>");
            return;
        }
        if (args.size() == 1)
            run_speed = convert_string<float>(args[0]);
        send_message_f("Running speed is %f", run_speed);
    } else if (cmd == "js") {
        if (args.size() > 1) {
            send_message_f("[!] Usage: js <jump_speed>");
            return;
        }
        if (args.size() == 1)
            jump_speed = convert_string<float>(args[0]);
        send_message_f("Jump speed is %f", jump_speed);
    } else if (cmd == "p") {
        send_message_f("Player = 0x%p 0x%p", player);
    } else if (cmd == "getpos") {
        if (!player) {
			send_message_f("[!] No player object set");
			return;
		}
		Vector3 pos;
		CALL_ORIG_VOID(Actor_GetPosition, actor_from_player(player), &pos);
		send_message_f("[*] Player position: %f %f %f", pos.x, pos.y, pos.z);
    } else if (cmd == "setrelpos") {
        if (!player) {
			send_message_f("[!] No player object set");
			return;
		}
		if (args.size() != 3) {
            send_message_f("[!] Usage: setrelpos dx dy dz");
            return;
        }
		float x = convert_string<float>(args[0]);
		float y = convert_string<float>(args[1]);
		float z = convert_string<float>(args[2]);
		Vector3 pos;
		CALL_ORIG_VOID(Actor_GetPosition, actor_from_player(player), &pos);
		send_message_f("[*] Old player position: %f %f %f", pos.x, pos.y, pos.z);
		pos.x += x;
		pos.y += y;
		pos.z += z;
		send_message_f("[*] New player position: %f %f %f", pos.x, pos.y, pos.z);
		CALL_ORIG_VOID(Actor_SetPosition, actor_from_player(player), &pos);
    } else if (cmd == "setpos") {
		if (!player) {
			send_message_f("[!] No player object set");
			return;
		}
		if (args.size() != 3) {
            send_message_f("[!] Usage: setpos dx dy dz");
            return;
        }
		Vector3 pos;
		pos.x = convert_string<float>(args[0]);
		pos.y = convert_string<float>(args[1]);
		pos.z = convert_string<float>(args[2]);
		send_message_f("[*] New player position: %f %f %f", pos.x, pos.y, pos.z);
		CALL_ORIG_VOID(Actor_SetPosition, actor_from_player(player), &pos);
	} else if (cmd == "freeze") {
		if (!player) {
			send_message_f("[!] No player object set");
			return;
		}
		if (args.size() != 4) {
            send_message_f("[!] Usage: freeze dx dy dz freq");
            return;
        }
		Vector3 pos;
		freeze_pos.x = convert_string<float>(args[0]);
		freeze_pos.y = convert_string<float>(args[1]);
		freeze_pos.z = convert_string<float>(args[2]);
		freeze_freq = convert_string<int>(args[3]);
		send_message_f("[*] Freezing player position: %f %f %f (interval %d ms)", pos.x, pos.y, pos.z, freeze_freq);
		freeze_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)freeze_loop, NULL, 0, NULL);
	} else if (cmd == "unfreeze") {
		if (freeze_thread)
			TerminateThread(freeze_thread, 0);
		freeze_thread = NULL;
	} else if (cmd == "travel") {
		if (args.size() != 1) {
			send_message_f("[!] Usage: travel dest");
            return;
		}
		next_travel_dest = strdup(args[0].c_str());
		send_message_f("[*] Next fast travel will get you to %s", next_travel_dest);
	}
}