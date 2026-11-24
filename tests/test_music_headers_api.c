static void reset_state(void) {
    memset(&__jo_audio_music, 0, sizeof __jo_audio_music);
    __jo_audio_music.volume = 76;
    __jo_audio_music_pending = false;
    reads = opens = closes = 0;
    lookup_missing = open_failure = false;
    read_length = 2048;
}

static int same_prepared(const jo_audio_prepared_music *a, const jo_audio_prepared_music *b) {
    return a->fad == b->fad && a->data_start == b->data_start && a->data_end == b->data_end &&
        a->rate == b->rate && a->channels == b->channels && a->format == b->format &&
        a->block_align == b->block_align && a->samples_per_block == b->samples_per_block &&
        a->frame_size == b->frame_size && a->prefix == b->prefix && a->prefix_bytes == b->prefix_bytes;
}

int main(int argc, char **argv) {
    assert(argc == 3);
    unsigned cases = 0, ordinary_reads = 0, supplied_reads = 0;
    const int red = atoi(argv[2]);
    for (unsigned cue = 1; cue <= 18; ++cue) {
        char path[4096], name[16];
        snprintf(name, sizeof name, "MU%02X.WAV", cue);
        snprintf(path, sizeof path, "%s/%s", argv[1], name);
        FILE *f = fopen(path, "rb"); assert(f);
        assert(!fseek(f, 0, SEEK_END)); actual_size = (unsigned)ftell(f);
        rewind(f); assert(fread(file_sector, 1, sizeof file_sector, f) == sizeof file_sector); fclose(f);
        actual_fad = 1000 + cue * 10000;
        unsigned char header[2048]; memcpy(header, file_sector, sizeof header);
        jo_audio_prepared_music expected, got;
        reset_state(); memset(&expected, 0, sizeof expected);
        assert(jo_audio_prepare_music_reference(name, &expected));
        assert(reads == 1 && opens == 1 && closes == 1); ordinary_reads += reads;
        reset_state(); assert(jo_audio_prepare_music(name, &got));
        assert(same_prepared(&got, &expected));
        reset_state(); memset(&got, 0, sizeof got);
        assert(supplied(name, header, 44, actual_size, &got));
        assert(same_prepared(&got, &expected)); supplied_reads += reads; ++cases;
        if (red) continue;
        assert(reads == 0 && opens == 1 && closes == 1);
        assert(__jo_audio_music.volume == 76);
        for (unsigned kind = 0; kind < 14; ++kind) {
            reset_state(); memcpy(header, file_sector, sizeof header);
            memset(&got, 0xA5, sizeof got); jo_audio_prepared_music untouched = got;
            unsigned n = 44, expected_size = actual_size;
            const char *use_name = name; const unsigned char *use_header = header;
            switch (kind) {
                case 0: use_name = NULL; break;
                case 1: use_header = NULL; break;
                case 2: n = 0; break;
                case 3: n = 43; break;
                case 4: n = 2049; break;
                case 5: ++expected_size; break;
                case 6: lookup_missing = true; break;
                case 7: open_failure = true; break;
                case 8: __jo_audio_music.playing = true; break;
                case 9: __jo_audio_music_pending = true; break;
                case 10: header[0] = 'X'; break;
                case 11: header[22] = 8; break;
                case 12: header[24] = header[25] = header[26] = header[27] = 0; break;
                case 13: header[40] = header[41] = header[42] = header[43] = 0; break;
            }
            assert(!supplied(use_name, use_header, n, expected_size, &got));
            assert(!memcmp(&got, &untouched, sizeof got)); assert(reads == 0); assert(opens == closes); ++cases;
        }
        reset_state(); assert(!supplied(name, header, 44, actual_size, NULL)); assert(!reads && !opens); ++cases;
        /* Full-sector and exact-prefix representations share the parser. */
        reset_state(); assert(supplied(name, file_sector, sizeof file_sector, actual_size, &got));
        assert(same_prepared(&got, &expected)); ++cases;
        /* Corruptions must retain the ordinary parser's acceptance and fields,
         * including its intentionally permissive RIFF identification. */
        for (unsigned byte = 0; byte < 44; ++byte) for (unsigned value = 0; value < 3; ++value) {
            const unsigned char values[] = {0, 0x80, 0xFF};
            unsigned char saved = file_sector[byte]; file_sector[byte] = values[value];
            reset_state(); memset(&expected, 0xA5, sizeof expected);
            bool ordinary_ok = jo_audio_prepare_music_reference(name, &expected);
            reset_state(); memset(&got, 0xA5, sizeof got);
            bool supplied_ok = supplied(name, file_sector, 44, actual_size, &got);
            assert(ordinary_ok == supplied_ok); assert(same_prepared(&got, &expected));
            assert(reads == 0); file_sector[byte] = saved; ++cases;
        }
        /* Ordinary API still fails on I/O, with output untouched. */
        reset_state(); read_length = -1; memset(&got, 0xA5, sizeof got);
        jo_audio_prepared_music unchanged = got;
        assert(!jo_audio_prepare_music(name, &got)); assert(!memcmp(&got, &unchanged, sizeof got)); ++cases;
        const unsigned short_reads[] = {1,11,12,35,36,43,44};
        for(unsigned j=0;j<sizeof short_reads/sizeof short_reads[0];++j) {
            reset_state();read_length=(int)short_reads[j];memset(&expected,0xA5,sizeof expected);
            bool before=jo_audio_prepare_music_reference(name,&expected);
            reset_state();read_length=(int)short_reads[j];memset(&got,0xA5,sizeof got);
            bool after=jo_audio_prepare_music(name,&got);
            assert(before==after && same_prepared(&got,&expected));assert(before==(short_reads[j]==44));++cases;
        }
    }
    printf("{\"cases\":%u,\"ordinary_reads\":%u,\"supplied_reads\":%u,\"target_zero_reads\":%s}\n",
           cases, ordinary_reads, supplied_reads, supplied_reads == 0 ? "true" : "false");
    return supplied_reads != 0;
}
