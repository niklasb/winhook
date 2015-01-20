struct Player {
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
    return jump_speed;
END_HOOK

Hook* hooks[] = {
    &Player_CanJump_hook,
    &Player_GetJumpSpeed_hook,
    &Player_GetWalkingSpeed_hook,
};

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
        send_message_f("Player = 0x%p", player);
    }
}