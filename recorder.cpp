#include <iostream>

  using namespace std;

#include "recorder.h"

Recorder::Recorder(const char *device_name, unsigned int _rate, snd_pcm_format_t _format, long _buffer_frames)
:format(_format), rate(_rate), buffer_frames(_buffer_frames) {
  cout << "Setting audio device..." << endl;

  int err = 0;

  if((err = snd_pcm_open(&(this->device), device_name, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fprintf(stderr, "cannot open audio device %s (%s)\n", 
             device_name,
             snd_strerror(err));
    throw string("cannot open audio device");
  }

  cout << "audio interface opened" << endl;
		   
  if((err = snd_pcm_hw_params_malloc(&(this->hw_params))) < 0) {
    fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror(err));
    throw string("cannot allocate hardware parameter structure");
  }

  cout << "hw_params allocated" << endl;
				 
  if((err = snd_pcm_hw_params_any(this->device, this->hw_params)) < 0) {
    fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror(err));
    throw string("cannot initialize hardware parameter structure");
  }

  cout << "hw_params initialized" << endl;
	
  if((err = snd_pcm_hw_params_set_access(this->device, this->hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf(stderr, "cannot set access type (%s)\n",
             snd_strerror(err));
    throw string("cannot set access type");
  }

  cout << "hw_params access setted" << endl;
	
  if((err = snd_pcm_hw_params_set_format(this->device, this->hw_params, this->format)) < 0) {
    fprintf(stderr, "cannot set sample format (%s)\n",
             snd_strerror(err));
    throw string("cannot set sample format");
  }

  cout << "hw_params format setted" << endl;
	
  if((err = snd_pcm_hw_params_set_rate_near(this->device, this->hw_params, &(this->rate), 0)) < 0) {
    fprintf(stderr, "cannot set sample rate (%s)\n",
             snd_strerror(err));
    throw string("cannot set sample rate");
  }
	
  cout << "hw_params rate setted" << endl;

  if((err = snd_pcm_hw_params_set_channels(this->device, this->hw_params, 1)) < 0) {
    fprintf(stderr, "cannot set channel count (%s)\n",
             snd_strerror(err));
    throw string("cannot set channel count");
  }

  cout << "hw_params channels setted" << endl;

  // snd_pcm_uframes_t periodsize = 256;
  // if((err = snd_pcm_hw_params_set_buffer_size_near(this->device, this->hw_params, &periodsize)) < 0) {
  //   fprintf(stderr, "cannot set channel count (%s)\n",
  //            snd_strerror(err));
  //   throw string("cannot set channel count");
  // }

  // cout << "hw_params channels setted" << endl;
	
  if((err = snd_pcm_hw_params(this->device, this->hw_params)) < 0) {
    fprintf(stderr, "cannot set parameters (%s)\n",
             snd_strerror(err));
    throw string("cannot set parameters");
  }

  cout << "hw_params setted" << endl;
	
  snd_pcm_hw_params_free(this->hw_params);

  cout << "hw_params freed" << endl;
	
  if((err = snd_pcm_prepare(this->device)) < 0) {
    fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror(err));
    throw string("cannot prepare audio interface for use");
  }

  this->buffer = (uint8_t *)malloc(this->buffer_frames * (snd_pcm_format_width(this->format) / 8));

  cout << "Recorder capture device ready" << endl;
}

unsigned int Recorder::captureBloc(uint16_t *&sink) {
  int err = 0;

  if((err = snd_pcm_readi(this->device, this->buffer, this->buffer_frames)) != this->buffer_frames) {
    fprintf (stderr, "read from audio interface failed (%s)\n",
              snd_strerror(err));
    throw string("cannot read from audio interface");
  }

  try {
    sink = new uint16_t [this->buffer_frames];
  }
  catch(const std::exception& e) {
    cerr << e.what() << endl;
  }

  for(unsigned int j = 0; j < this->buffer_frames; j+=2) {
    sink[j] = (uint16_t)(this->buffer[j] << 8) | (this->buffer[j+1]);
  }

  return this->buffer_frames;
}

// void Recorder::recordToWaveFile(const char *filename) {
//   int err = 0;

//   WaveHeader *hdr = genericWAVHeader(96000, 16, 1);
// uint32_t pcm_data_size = hdr->sample_rate * hdr->bytes_per_frame * duration / 1000;
//   hdr->file_size

//   int fd = open(filename, O_WRONLY | O_CREAT, 0644);
//   err = writeWAVHeader(fd, hdr);

//   for(unsigned int i = 0; i < 1525; ++i) {
//     if((err = snd_pcm_readi(this->device, this->buffer, this->buffer_frames)) != this->buffer_frames) {
//       fprintf (stderr, "read from audio interface failed (%s)\n",
//                snd_strerror(err));
//       throw string("cannot read from audio interface");
//     }

//     write(fd, this->buffer, this->buffer_frames * (snd_pcm_format_width(this->format) / 8));
//   }
// }

WaveHeader* Recorder::genericWAVHeader(uint32_t sample_rate, uint16_t bit_depth, uint16_t channels) {
  WaveHeader *hdr;
  hdr = (WaveHeader*) malloc(sizeof(*hdr));
  if(!hdr) {
    return NULL;
  }

  memcpy(&hdr->RIFF_marker, "RIFF", 4);
  memcpy(&hdr->filetype_header, "WAVE", 4);
  memcpy(&hdr->format_marker, "fmt ", 4);
  hdr->data_header_length = 16;
  hdr->format_type = 1;
  hdr->number_of_channels = channels;
  hdr->sample_rate = sample_rate;
  hdr->bytes_per_second = sample_rate * channels * bit_depth / 8;
  hdr->bytes_per_frame = channels * bit_depth / 8;
  hdr->bits_per_sample = bit_depth;

  return hdr;
}

int Recorder::writeWAVHeader(int fd, WaveHeader *hdr) {
  if(!hdr) {
    return -1;
  }

  write(fd, &hdr->RIFF_marker, 4);
  write(fd, &hdr->file_size, 4);
  write(fd, &hdr->filetype_header, 4);
  write(fd, &hdr->format_marker, 4);
  write(fd, &hdr->data_header_length, 4);
  write(fd, &hdr->format_type, 2);
  write(fd, &hdr->number_of_channels, 2);
  write(fd, &hdr->sample_rate, 4);
  write(fd, &hdr->bytes_per_second, 4);
  write(fd, &hdr->bytes_per_frame, 2);
  write(fd, &hdr->bits_per_sample, 2);
  write(fd, "data", 4);

  uint32_t data_size = hdr->file_size + 8 - 44;
  write(fd, &data_size, 4);

  return 0;
}

Recorder::~Recorder() {
  cout << "Destroying instance of Recorder" << endl;

  free(this->buffer);
  snd_pcm_close(this->device);
}
