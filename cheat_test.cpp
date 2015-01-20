DECLARE_HOOK("test.exe", 0x1a10, int, foo, int x)
    cout << "x = " << x << endl;
    return CALL_ORIG(foo, x);
END_HOOK

DECLARE_HOOK("test.exe", 0x1a50, void, bar, int y)
    cout << "y = " << y << endl;
    CALL_ORIG_VOID(bar, y);
END_HOOK

DECLARE_HOOK_THISCALL("test.exe", 0x1c20, int, test, Player*, int a, int b, int c)
    cout << "this->d = " << this_->d << endl;
    cout << "a b c = " << a << " " << b << " " << c << endl;
    return CALL_ORIG(test, this_, a, b, c);
END_HOOK

Hook* hooks[] = {
    &foo_hook,
    &bar_hook,
    &test_hook,
};

void handle_user_command(string cmd, vector<string> args) {
}