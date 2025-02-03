#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <iostream>
#include <nan.h>

typedef enum {
    AUDIO_VOLUME_SET,
    AUDIO_VOLUME_GET,
    AUDIO_MUTE,
    AUDIO_UNMUTE,
} audio_volume_action;

snd_mixer_t *handle;

snd_mixer_elem_t* getMixerElem() {

  snd_mixer_selem_id_t *sid;
  const char *card = "internal";
  const char *selem_name = "PCM";

  snd_mixer_open(&handle, 0);
  snd_mixer_attach(handle, card);
  snd_mixer_selem_register(handle, NULL, NULL);
  snd_mixer_load(handle);

  snd_mixer_selem_id_alloca(&sid);
  snd_mixer_selem_id_set_index(sid, 0);
  snd_mixer_selem_id_set_name(sid, selem_name);

  snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
  return elem;
}

float getVolume() {
  long min, max;
  long volume = 0;

  snd_mixer_elem_t* elem = getMixerElem();

  snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
  snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &volume);

  snd_mixer_close(handle);

  return volume / (1.f * max);
}

// volume 0.0 - 1.0
void setVolumeLevel(float volume) {

  long outvol = volume * 100;
  long min, max;

  snd_mixer_elem_t* elem = getMixerElem();

  snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

  outvol = outvol * (max - min) / (100-1) + min;
  snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, outvol);
  snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, outvol);

  snd_mixer_close(handle);
}


void setMuteState(bool muted) {
  snd_mixer_elem_t* elem = getMixerElem();

  if (muted) {
    snd_mixer_selem_set_playback_switch_all(elem, 0);
  } else {
    snd_mixer_selem_set_playback_switch_all(elem, 1);
  }
  snd_mixer_close(handle);
}

bool isMuted() {
  int leftMuted = 0;
  int rightMuted = 0;

  snd_mixer_elem_t* elem = getMixerElem();

  snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &leftMuted);
  snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_RIGHT, &rightMuted);

  snd_mixer_close(handle);
  return leftMuted + rightMuted == 0;
}

float setVolume(float vol) {
  float newVolume = vol;


  if (newVolume > 1.0) {
    newVolume = 1.0;
  }

  if (newVolume < 0) {
    newVolume = 0;
  }

  setVolumeLevel(newVolume);
  return getVolume();
}

bool setMute(bool muted) {
  setMuteState(muted);
  return muted;
}

NAN_METHOD(GetVolume) {

  info.GetReturnValue().Set(Nan::New(getVolume()));
}

NAN_METHOD(SetVolume) {
  float newVolume = (float)info[0]->NumberValue(Nan::GetCurrentContext()).ToChecked();
  //float newVolume = (float)Nan::To<v8::Number>(info[0]).ToLocalChecked(); // This one is borked. Not sure how to do this right.
  setVolume(newVolume);
  // info.GetReturnValue().Set(Nan::New(setVolume(newVolume)));
}

NAN_METHOD(IsMuted) {
  info.GetReturnValue().Set(Nan::New(isMuted()));
}

NAN_METHOD(SetMute) {
  bool muted = (bool)Nan::To<bool>(info[0]).ToChecked();
  setMute(muted);
  info.GetReturnValue().Set(Nan::New(isMuted()));
}

NAN_MODULE_INIT(init) {
  target->Set(  Nan::GetCurrentContext(),
                Nan::New("getVolume").ToLocalChecked(),
                Nan::GetFunction(Nan::New<v8::FunctionTemplate>(GetVolume)).ToLocalChecked());
  target->Set(  Nan::GetCurrentContext(),
                Nan::New("setVolume").ToLocalChecked(),
                Nan::GetFunction(Nan::New<v8::FunctionTemplate>(SetVolume)).ToLocalChecked());
  target->Set(  Nan::GetCurrentContext(),
                Nan::New("isMuted").ToLocalChecked(),
                Nan::GetFunction(Nan::New<v8::FunctionTemplate>(IsMuted)).ToLocalChecked());
  target->Set(  Nan::GetCurrentContext(),
                Nan::New("setMute").ToLocalChecked(),
                Nan::GetFunction(Nan::New<v8::FunctionTemplate>(SetMute)).ToLocalChecked());
}

NODE_MODULE(addon, init)
