#define main video_bus_main
#include "video_bus_probe.c"
#undef main
#define main combat_join_main
#include "combat_join_probe.c"
#undef main
int main(void){return video_bus_main()||combat_join_main();}
