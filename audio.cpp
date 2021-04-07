#include <iostream>

  using namespace std;

#include "audio.h"

Audio::Audio(const char *device_name, unsigned int _rate, snd_pcm_format_t _format, uint16_t _buffer_frames)
:format(_format), rate(_rate), buffer_frames(_buffer_frames) {
  cout << "Setting audio device..." << endl;

  int err = 0;

  if((err = snd_pcm_open (&(this->device), device_name, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
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
    throw string("ccannot set sample format");
  }

  cout << "hw_params format setted" << endl;
	
  if((err = snd_pcm_hw_params_set_rate_near(this->device, this->hw_params, &(this->rate), 0)) < 0) {
    fprintf(stderr, "cannot set sample rate (%s)\n",
             snd_strerror(err));
    throw string("cannot set sample rate");
  }
	
  cout << "hw_params rate setted" << endl;

  if((err = snd_pcm_hw_params_set_channels(this->device, this->hw_params, 2)) < 0) {
    fprintf(stderr, "cannot set channel count (%s)\n",
             snd_strerror(err));
    throw string("cannot set channel count");
  }

  cout << "hw_params channels setted" << endl;
	
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

  cout << "Audio capture device ready" << endl;
}

void Audio::capture() {
  int err = 0;

  for(unsigned int i = 0; i < 10; ++i) {
    if((err = snd_pcm_readi(this->device, this->buffer, this->buffer_frames)) != this->buffer_frames) {
      fprintf (stderr, "read from audio interface failed (%s)\n",
               snd_strerror(err));
      exit (1);
    }
    cout << "read " << i << " done" << endl;
  }
}

Audio::~Audio() {
  cout << "Destroying instance of Audio" << endl;

  free(this->buffer);
  snd_pcm_close(this->device);
}