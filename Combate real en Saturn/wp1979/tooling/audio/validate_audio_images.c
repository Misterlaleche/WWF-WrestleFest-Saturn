#include <stdio.h>
#include <stdlib.h>
#include "wf_native_elf.h"
#include "wf_music_elf.h"

static void *read_file(const char *path, size_t *bytes)
{
    FILE *f = fopen(path, "rb");
    if (!f || fseek(f, 0, SEEK_END)) return NULL;
    long size = ftell(f);
    if (size <= 0 || fseek(f, 0, SEEK_SET)) return NULL;
    void *data = malloc((size_t)size);
    if (!data || fread(data, 1, (size_t)size, f) != (size_t)size) return NULL;
    fclose(f);
    *bytes = (size_t)size;
    return data;
}

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    size_t scsp_bytes, music_bytes;
    void *scsp = read_file(argv[1], &scsp_bytes);
    void *music = read_file(argv[2], &music_bytes);
    if (!scsp || !music) return 3;
    wf_native_segment_t segments[3];
    wf_native_image_t image;
    wf_music_image_t module;
    int scsp_ok = wf_native_elf_image(scsp, scsp_bytes, segments, &image);
    int music_ok = wf_music_elf_image(music, music_bytes, &module);
    printf("SCSP production loader: %s\nMUSIC production loader: %s\n",
           scsp_ok ? "PASS" : "FAIL", music_ok ? "PASS" : "FAIL");
    free(scsp);
    free(music);
    return scsp_ok && music_ok ? 0 : 1;
}
