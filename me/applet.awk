BEGIN   { print "// Automatically generated.\n\n#include \"../applet.h\"\n"; }
/ applet/       { print "const uintptr_t "$3"_addr = 0x"$1";"; }
/__max_task__/  { print "const unsigned applet_max_task = 0x"$1";"; }
