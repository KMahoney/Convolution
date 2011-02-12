
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include <assert.h>

sf_count_t info_n(SF_INFO* info) {
  return info->frames * info->channels;
}

SNDFILE* read_wave(char* filename, SF_INFO* info) {
  SNDFILE* snd;
  info->format = 0;
  snd = sf_open(filename, SFM_READ, info);
  if (!snd) { fprintf(stderr, "%s: %s\n", filename, sf_strerror(snd)); exit(EXIT_FAILURE); }
  printf("%s: %d - %d channel\n", filename, info->samplerate, info->channels);
  return snd;
}

float* read_whole_buffer(SNDFILE* snd, SF_INFO* info) {
  sf_count_t count = info_n(info);
  float* buffer = malloc(sizeof(float) * count);
  sf_read_float(snd, buffer, count);
  return buffer;
}

void convolve_wave(char* infile, char* iirfile, char* outfile) {
  SF_INFO in_info, iir_info;

  SNDFILE* in_snd = read_wave(infile, &in_info);
  SNDFILE* iir_snd = read_wave(iirfile, &iir_info);

  float* in_buffer = read_whole_buffer(in_snd, &in_info);
  float* iir_buffer = read_whole_buffer(iir_snd, &iir_info);

  sf_count_t in_len = info_n(&in_info);
  sf_count_t iir_len = info_n(&iir_info);
  sf_count_t out_len = in_len + iir_len - 1;

  float* out = malloc(sizeof(float) * out_len);

  // convolution
  printf("Convolution.\n");
  float val, max;
  sf_count_t low, high, i, j;

  // BEWARE OFF BY ONE ERRORS
  max = 0.0;
  for (i = 0; i < out_len; i++) {
    low = i - iir_len + 1;
    low = low > 0 ? low : 0;

    high = i;
    high = high > (in_len-1) ? (in_len-1) : high;

    val = 0;
    for (j = low; j <= high; j++) {
      sf_count_t k = (i-low)-(j-low);
      //assert(j >= 0 && j < in_len);
      //assert(k >= 0 && k < iir_len);
      val += in_buffer[j] * iir_buffer[k];
    }

    if (abs(val) > max) max = abs(val);
    out[i] = val;
  }

  free(in_buffer);
  free(iir_buffer);

  sf_close(in_snd);
  sf_close(iir_snd);

  // normalise
  printf("Normalisation.\n");
  for (i = 0; i < out_len; i++) out[i] = out[i] / max;

  // write it
  SF_INFO out_info;
  memcpy(&out_info, &in_info, sizeof(SF_INFO));
  out_info.frames = out_len / out_info.channels;
  printf("writing %d frames.\n", out_info.frames);

  SNDFILE* out_snd = sf_open(outfile, SFM_WRITE, &out_info);
  sf_write_float(out_snd, out, out_len);
  sf_close(out_snd);
  free(out);
}

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "Expecting filenames: Input, IIR, Output.\n");
    return EXIT_FAILURE;
  }

  convolve_wave(argv[1], argv[2], argv[3]);
  return EXIT_SUCCESS;
}
