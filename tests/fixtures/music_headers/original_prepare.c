/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2026, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Johannes Fetz nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/* Frozen pre-optimization parser/prepare bodies; see provenance.json. */

static  __jo_force_inline unsigned int  reference_wav_value(const unsigned char * const p, const int bytes)
{
    return (bytes == 2 ? (unsigned int)(p[0] | (p[1] << 8)) : (unsigned int)(p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned int)p[3] << 24)));
}

static unsigned int                 reference_wav_format(const unsigned char * const p, const unsigned char * const end)
{
    unsigned int                    format;

    format = reference_wav_value(p + 8, 2);
    if (format == 0xFFFE && reference_wav_value(p + 4, 4) >= 40 && p + 34 <= end)
        format = reference_wav_value(p + 32, 2);
    return (format);
}

static bool                         reference_music_header(const unsigned char * const header, const unsigned int size)
{
    const unsigned char             *p;
    const unsigned char             *end;
    unsigned int                    format;
    unsigned int                    bits;
    unsigned int                    chunk;

    if (size < 12 || header[0] != 'R' || header[1] != 'I' || header[8] != 'W' || header[9] != 'A')
        return (false);
    JO_ZERO(format);
    JO_ZERO(bits);
    end = header + size;
    for (p = header + 12; p + 8 <= end; p += 8 + ((chunk + 1) & ~1U))
    {
        chunk = reference_wav_value(p + 4, 4);
        if (p[0] == 'f' && p[1] == 'm' && p[2] == 't' && p + 24 <= end)
        {
            format = reference_wav_format(p, end);
            __jo_audio_music.channels = reference_wav_value(p + 10, 2);
            __jo_audio_music.rate = reference_wav_value(p + 12, 4);
            __jo_audio_music.block_align = reference_wav_value(p + 20, 2);
            bits = reference_wav_value(p + 22, 2);
        }
        else if (p[0] == 'd' && p[1] == 'a' && p[2] == 't' && p[3] == 'a')
        {
            __jo_audio_music.data_start = (unsigned int)(p + 8 - header);
            // (a size of 0xFFFFFFFF: to the end of the file, see jo_audio_play_music())
            __jo_audio_music.data_end = chunk > 0xFFFFFFFFu - __jo_audio_music.data_start ? 0xFFFFFFFFu :
                                        __jo_audio_music.data_start + chunk;
            break;
        }
        // (a chunk after the end of the header read: the end, not a pointer wrapped around)
        if (chunk > (unsigned int)(end - p) - 8)
            break;
    }
    if (__jo_audio_music.data_end == 0 || (__jo_audio_music.channels != 1 && __jo_audio_music.channels != 2) ||
        __jo_audio_music.rate == 0 || __jo_audio_music.rate > 2 * JO_SCSP_BASE_SAMPLING_RATE)
        return (false);
    if (format == 1 && (bits == 8 || bits == 16))
    {
        __jo_audio_music.format = bits == 8 ? JoAudioMusicPcm8 : JoAudioMusicPcm16;
        __jo_audio_music.block_align = __jo_audio_music.channels * (bits >> 3);
        __jo_audio_music.frame_size = __jo_audio_music.block_align;
    }
    else if (format == 0x11 && bits == 4 && __jo_audio_music.block_align > 4 * __jo_audio_music.channels &&
             __jo_audio_music.block_align <= JO_AUDIO_MUSIC_MAX_BLOCK && (__jo_audio_music.block_align & 3) == 0)
    {
        __jo_audio_music.format = JoAudioMusicImaAdpcm;
        __jo_audio_music.samples_per_block = (__jo_audio_music.block_align - 4 * __jo_audio_music.channels) * 2 / __jo_audio_music.channels + 1;
    }
    else
        return (false);
    __jo_audio_music.data_end -= (__jo_audio_music.data_end - __jo_audio_music.data_start) % __jo_audio_music.block_align;
    return (__jo_audio_music.data_end > __jo_audio_music.data_start);
}

bool jo_audio_prepare_music_reference(const char *filename, jo_audio_prepared_music *prepared)
{
    void *handle;
    int file_id, len;
    unsigned int size;
    if (!filename || !prepared || __jo_audio_music.playing || __jo_audio_music_pending) return false;
    if ((file_id = __jo_fs_get_file_id(filename)) < 0 || (handle = __jo_fs_open(file_id)) == JO_NULL) return false;
    __jo_audio_music.fad = __jo_fs_get_fad(handle);
    size = (unsigned int)__jo_fs_get_file_size(handle);
    __jo_audio_music.data_start = __jo_audio_music.data_end = 0;
    len = __jo_fs_read_sectors(handle, 1, __jo_audio_music_sector, JO_AUDIO_MUSIC_SECTOR_SIZE);
    __jo_fs_close(handle);
    if (len <= 0 || !reference_music_header((unsigned char *)__jo_audio_music_sector, (unsigned int)len)) return false;
    /* Prepared asynchronous starts are deliberately limited to PCM. Compressed
     * startup needs its own frame availability calculation and is not used here. */
    if (__jo_audio_music.format != JoAudioMusicPcm16 || size <= __jo_audio_music.data_start) return false;
    if (__jo_audio_music.data_end > size)
        __jo_audio_music.data_end = size - (size - __jo_audio_music.data_start) % __jo_audio_music.block_align;
    *prepared = (jo_audio_prepared_music){__jo_audio_music.fad, __jo_audio_music.data_start,
        __jo_audio_music.data_end, __jo_audio_music.rate, __jo_audio_music.channels,
        (unsigned int)__jo_audio_music.format, __jo_audio_music.block_align,
        __jo_audio_music.samples_per_block, __jo_audio_music.frame_size, JO_NULL, 0};
    return true;
}
