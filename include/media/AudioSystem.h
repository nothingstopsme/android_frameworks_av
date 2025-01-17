/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_AUDIOSYSTEM_H_
#define ANDROID_AUDIOSYSTEM_H_

#include <hardware/audio_effect.h>
#include <media/AudioPolicy.h>
#include <media/AudioIoDescriptor.h>
#include <media/IAudioFlingerClient.h>
#include <media/IAudioPolicyServiceClient.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <utils/Errors.h>
#include <utils/Mutex.h>

namespace android {

typedef void (*audio_error_callback)(status_t err);
typedef void (*dynamic_policy_callback)(int event, String8 regId, int val);
typedef void (*record_config_callback)(int event, audio_session_t session, int source,
                const audio_config_base_t *clientConfig, const audio_config_base_t *deviceConfig,
                audio_patch_handle_t patchHandle);
typedef void (*audio_session_callback)(int event,
        sp<AudioSessionInfo>& session, bool added);

class IAudioFlinger;
class IAudioPolicyService;
class String8;

class AudioSystem
{
public:

    // FIXME Declare in binder opcode order, similarly to IAudioFlinger.h and IAudioFlinger.cpp

    /* These are static methods to control the system-wide AudioFlinger
     * only privileged processes can have access to them
     */

    // mute/unmute microphone
    static status_t muteMicrophone(bool state);
    static status_t isMicrophoneMuted(bool *state);

    // set/get master volume
    static status_t setMasterVolume(float value);
    static status_t getMasterVolume(float* volume);

    // mute/unmute audio outputs
    static status_t setMasterMute(bool mute);
    static status_t getMasterMute(bool* mute);

    // set/get stream volume on specified output
    static status_t setStreamVolume(audio_stream_type_t stream, float value,
                                    audio_io_handle_t output);
    static status_t getStreamVolume(audio_stream_type_t stream, float* volume,
                                    audio_io_handle_t output);

    // mute/unmute stream
    static status_t setStreamMute(audio_stream_type_t stream, bool mute);
    static status_t getStreamMute(audio_stream_type_t stream, bool* mute);

    // set audio mode in audio hardware
    static status_t setMode(audio_mode_t mode);

    // returns true in *state if tracks are active on the specified stream or have been active
    // in the past inPastMs milliseconds
    static status_t isStreamActive(audio_stream_type_t stream, bool *state, uint32_t inPastMs);
    // returns true in *state if tracks are active for what qualifies as remote playback
    // on the specified stream or have been active in the past inPastMs milliseconds. Remote
    // playback isn't mutually exclusive with local playback.
    static status_t isStreamActiveRemotely(audio_stream_type_t stream, bool *state,
            uint32_t inPastMs);
    // returns true in *state if a recorder is currently recording with the specified source
    static status_t isSourceActive(audio_source_t source, bool *state);

    // set/get audio hardware parameters. The function accepts a list of parameters
    // key value pairs in the form: key1=value1;key2=value2;...
    // Some keys are reserved for standard parameters (See AudioParameter class).
    // The versions with audio_io_handle_t are intended for internal media framework use only.
    static status_t setParameters(audio_io_handle_t ioHandle, const String8& keyValuePairs);
    static String8  getParameters(audio_io_handle_t ioHandle, const String8& keys);
    // The versions without audio_io_handle_t are intended for JNI.
    static status_t setParameters(const String8& keyValuePairs);
    static String8  getParameters(const String8& keys);

    static void setErrorCallback(audio_error_callback cb);
    static void setDynPolicyCallback(dynamic_policy_callback cb);
    static void setRecordConfigCallback(record_config_callback);
    static status_t setAudioSessionCallback(audio_session_callback cb);

    // helper function to obtain AudioFlinger service handle
    static const sp<IAudioFlinger> get_audio_flinger();

    static float linearToLog(int volume);
    static int logToLinear(float volume);

    // Returned samplingRate and frameCount output values are guaranteed
    // to be non-zero if status == NO_ERROR
    // FIXME This API assumes a route, and so should be deprecated.
    static status_t getOutputSamplingRate(uint32_t* samplingRate,
            audio_stream_type_t stream);
    // FIXME This API assumes a route, and so should be deprecated.
    static status_t getOutputFrameCount(size_t* frameCount,
            audio_stream_type_t stream);
    // FIXME This API assumes a route, and so should be deprecated.
    static status_t getOutputLatency(uint32_t* latency,
            audio_stream_type_t stream);
    // returns the audio HAL sample rate
    static status_t getSamplingRate(audio_io_handle_t ioHandle,
                                          uint32_t* samplingRate);
    // For output threads with a fast mixer, returns the number of frames per normal mixer buffer.
    // For output threads without a fast mixer, or for input, this is same as getFrameCountHAL().
    static status_t getFrameCount(audio_io_handle_t ioHandle,
                                  size_t* frameCount);
    // returns the audio output latency in ms. Corresponds to
    // audio_stream_out->get_latency()
    static status_t getLatency(audio_io_handle_t output,
                               uint32_t* latency);

    // return status NO_ERROR implies *buffSize > 0
    // FIXME This API assumes a route, and so should deprecated.
    static status_t getInputBufferSize(uint32_t sampleRate, audio_format_t format,
        audio_channel_mask_t channelMask, size_t* buffSize);

    static status_t setVoiceVolume(float volume);

    // return the number of audio frames written by AudioFlinger to audio HAL and
    // audio dsp to DAC since the specified output has exited standby.
    // returned status (from utils/Errors.h) can be:
    // - NO_ERROR: successful operation, halFrames and dspFrames point to valid data
    // - INVALID_OPERATION: Not supported on current hardware platform
    // - BAD_VALUE: invalid parameter
    // NOTE: this feature is not supported on all hardware platforms and it is
    // necessary to check returned status before using the returned values.
    static status_t getRenderPosition(audio_io_handle_t output,
                                      uint32_t *halFrames,
                                      uint32_t *dspFrames);

    // return the number of input frames lost by HAL implementation, or 0 if the handle is invalid
    static uint32_t getInputFramesLost(audio_io_handle_t ioHandle);

    // Allocate a new unique ID for use as an audio session ID or I/O handle.
    // If unable to contact AudioFlinger, returns AUDIO_UNIQUE_ID_ALLOCATE instead.
    // FIXME If AudioFlinger were to ever exhaust the unique ID namespace,
    //       this method could fail by returning either a reserved ID like AUDIO_UNIQUE_ID_ALLOCATE
    //       or an unspecified existing unique ID.
    static audio_unique_id_t newAudioUniqueId(audio_unique_id_use_t use);

    static void acquireAudioSessionId(audio_session_t audioSession, pid_t pid);
    static void releaseAudioSessionId(audio_session_t audioSession, pid_t pid);

    // Get the HW synchronization source used for an audio session.
    // Return a valid source or AUDIO_HW_SYNC_INVALID if an error occurs
    // or no HW sync source is used.
    static audio_hw_sync_t getAudioHwSyncForSession(audio_session_t sessionId);

    // Indicate JAVA services are ready (scheduling, power management ...)
    static status_t systemReady();

    // Returns the number of frames per audio HAL buffer.
    // Corresponds to audio_stream->get_buffer_size()/audio_stream_in_frame_size() for input.
    // See also getFrameCount().
    static status_t getFrameCountHAL(audio_io_handle_t ioHandle,
                                     size_t* frameCount);

    // Events used to synchronize actions between audio sessions.
    // For instance SYNC_EVENT_PRESENTATION_COMPLETE can be used to delay recording start until
    // playback is complete on another audio session.
    // See definitions in MediaSyncEvent.java
    enum sync_event_t {
        SYNC_EVENT_SAME = -1,             // used internally to indicate restart with same event
        SYNC_EVENT_NONE = 0,
        SYNC_EVENT_PRESENTATION_COMPLETE,

        //
        // Define new events here: SYNC_EVENT_START, SYNC_EVENT_STOP, SYNC_EVENT_TIME ...
        //
        SYNC_EVENT_CNT,
    };

    // Timeout for synchronous record start. Prevents from blocking the record thread forever
    // if the trigger event is not fired.
    static const uint32_t kSyncRecordStartTimeOutMs = 30000;

    //
    // IAudioPolicyService interface (see AudioPolicyInterface for method descriptions)
    //
    static status_t setDeviceConnectionState(audio_devices_t device, audio_policy_dev_state_t state,
                                             const char *device_address, const char *device_name);
		// the backward-compatible version of setDeviceConnectionState()
		static status_t setDeviceConnectionState(audio_devices_t device, audio_policy_dev_state_t state,
                                             const char *device_address);

    static audio_policy_dev_state_t getDeviceConnectionState(audio_devices_t device,
                                                                const char *device_address);
    static status_t setPhoneState(audio_mode_t state);
    static status_t setForceUse(audio_policy_force_use_t usage, audio_policy_forced_cfg_t config);
    static audio_policy_forced_cfg_t getForceUse(audio_policy_force_use_t usage);

    // Client must successfully hand off the handle reference to AudioFlinger via createTrack(),
    // or release it with releaseOutput().
    static audio_io_handle_t getOutput(audio_stream_type_t stream,
                                        uint32_t samplingRate = 0,
                                        audio_format_t format = AUDIO_FORMAT_DEFAULT,
                                        audio_channel_mask_t channelMask = AUDIO_CHANNEL_OUT_STEREO,
                                        audio_output_flags_t flags = AUDIO_OUTPUT_FLAG_NONE,
                                        const audio_offload_info_t *offloadInfo = NULL);
    static status_t getOutputForAttr(const audio_attributes_t *attr,
                                     audio_io_handle_t *output,
                                     audio_session_t session,
                                     audio_stream_type_t *stream,
                                     uid_t uid,
                                     uint32_t samplingRate = 0,
                                     audio_format_t format = AUDIO_FORMAT_DEFAULT,
                                     audio_channel_mask_t channelMask = AUDIO_CHANNEL_OUT_STEREO,
                                     audio_output_flags_t flags = AUDIO_OUTPUT_FLAG_NONE,
                                     audio_port_handle_t selectedDeviceId = AUDIO_PORT_HANDLE_NONE,
                                     const audio_offload_info_t *offloadInfo = NULL);
    static status_t startOutput(audio_io_handle_t output,
                                audio_stream_type_t stream,
                                audio_session_t session);
    static status_t stopOutput(audio_io_handle_t output,
                               audio_stream_type_t stream,
                               audio_session_t session);
    static void releaseOutput(audio_io_handle_t output,
                              audio_stream_type_t stream,
                              audio_session_t session);

    // Client must successfully hand off the handle reference to AudioFlinger via openRecord(),
    // or release it with releaseInput().
    static status_t getInputForAttr(const audio_attributes_t *attr,
                                    audio_io_handle_t *input,
                                    audio_session_t session,
                                    pid_t pid,
                                    uid_t uid,
                                    uint32_t samplingRate,
                                    audio_format_t format,
                                    audio_channel_mask_t channelMask,
                                    audio_input_flags_t flags,
                                    audio_port_handle_t selectedDeviceId = AUDIO_PORT_HANDLE_NONE);

    static status_t startInput(audio_io_handle_t input,
                               audio_session_t session);
    static status_t stopInput(audio_io_handle_t input,
                              audio_session_t session);
    static void releaseInput(audio_io_handle_t input,
                             audio_session_t session);
    static status_t initStreamVolume(audio_stream_type_t stream,
                                      int indexMin,
                                      int indexMax);
    static status_t setStreamVolumeIndex(audio_stream_type_t stream,
                                         int index,
                                         audio_devices_t device);
    static status_t getStreamVolumeIndex(audio_stream_type_t stream,
                                         int *index,
                                         audio_devices_t device);

    static uint32_t getStrategyForStream(audio_stream_type_t stream);
    static audio_devices_t getDevicesForStream(audio_stream_type_t stream);

    static audio_io_handle_t getOutputForEffect(const effect_descriptor_t *desc);
    static status_t registerEffect(const effect_descriptor_t *desc,
                                    audio_io_handle_t io,
                                    uint32_t strategy,
                                    audio_session_t session,
                                    int id);
    static status_t unregisterEffect(int id);
    static status_t setEffectEnabled(int id, bool enabled);

    // clear stream to output mapping cache (gStreamOutputMap)
    // and output configuration cache (gOutputs)
    static void clearAudioConfigCache();

    static const sp<IAudioPolicyService> get_audio_policy_service();

    // helpers for android.media.AudioManager.getProperty(), see description there for meaning
    static uint32_t getPrimaryOutputSamplingRate();
    static size_t getPrimaryOutputFrameCount();

    static status_t setLowRamDevice(bool isLowRamDevice);

    // Check if hw offload is possible for given format, stream type, sample rate,
    // bit rate, duration, video and streaming or offload property is enabled
    static bool isOffloadSupported(const audio_offload_info_t& info);

    // check presence of audio flinger service.
    // returns NO_ERROR if binding to service succeeds, DEAD_OBJECT otherwise
    static status_t checkAudioFlinger();

    /* List available audio ports and their attributes */
    static status_t listAudioPorts(audio_port_role_t role,
                                   audio_port_type_t type,
                                   unsigned int *num_ports,
                                   struct audio_port *ports,
                                   unsigned int *generation);

    /* Get attributes for a given audio port */
    static status_t getAudioPort(struct audio_port *port);

    /* Create an audio patch between several source and sink ports */
    static status_t createAudioPatch(const struct audio_patch *patch,
                                       audio_patch_handle_t *handle);

    /* Release an audio patch */
    static status_t releaseAudioPatch(audio_patch_handle_t handle);

    /* List existing audio patches */
    static status_t listAudioPatches(unsigned int *num_patches,
                                      struct audio_patch *patches,
                                      unsigned int *generation);
    /* Set audio port configuration */
    static status_t setAudioPortConfig(const struct audio_port_config *config);


    static status_t acquireSoundTriggerSession(audio_session_t *session,
                                           audio_io_handle_t *ioHandle,
                                           audio_devices_t *device);
    static status_t releaseSoundTriggerSession(audio_session_t session);

    static audio_mode_t getPhoneState();

    static status_t registerPolicyMixes(Vector<AudioMix> mixes, bool registration);

    static status_t startAudioSource(const struct audio_port_config *source,
                                      const audio_attributes_t *attributes,
                                      audio_io_handle_t *handle);
    static status_t stopAudioSource(audio_io_handle_t handle);

    static status_t setMasterMono(bool mono);
    static status_t getMasterMono(bool *mono);

    static status_t listAudioSessions(audio_stream_type_t streams,
                                      Vector< sp<AudioSessionInfo>> &sessions);

    // ----------------------------------------------------------------------------

    class AudioPortCallback : public RefBase
    {
    public:

                AudioPortCallback() {}
        virtual ~AudioPortCallback() {}

        virtual void onAudioPortListUpdate() = 0;
        virtual void onAudioPatchListUpdate() = 0;
        virtual void onServiceDied() = 0;

    };

    static status_t addAudioPortCallback(const sp<AudioPortCallback>& callback);
    static status_t removeAudioPortCallback(const sp<AudioPortCallback>& callback);

    class AudioDeviceCallback : public RefBase
    {
    public:

                AudioDeviceCallback() {}
        virtual ~AudioDeviceCallback() {}

        virtual void onAudioDeviceUpdate(audio_io_handle_t audioIo,
                                         audio_port_handle_t deviceId) = 0;
    };

    static status_t addAudioDeviceCallback(const sp<AudioDeviceCallback>& callback,
                                           audio_io_handle_t audioIo);
    static status_t removeAudioDeviceCallback(const sp<AudioDeviceCallback>& callback,
                                              audio_io_handle_t audioIo);

    static audio_port_handle_t getDeviceIdForIo(audio_io_handle_t audioIo);

private:

    class AudioFlingerClient: public IBinder::DeathRecipient, public BnAudioFlingerClient
    {
    public:
        AudioFlingerClient() :
            mInBuffSize(0), mInSamplingRate(0),
            mInFormat(AUDIO_FORMAT_DEFAULT), mInChannelMask(AUDIO_CHANNEL_NONE) {
        }

        void clearIoCache();
        status_t getInputBufferSize(uint32_t sampleRate, audio_format_t format,
                                    audio_channel_mask_t channelMask, size_t* buffSize);
        sp<AudioIoDescriptor> getIoDescriptor(audio_io_handle_t ioHandle);

        // DeathRecipient
        virtual void binderDied(const wp<IBinder>& who);

        // IAudioFlingerClient

        // indicate a change in the configuration of an output or input: keeps the cached
        // values for output/input parameters up-to-date in client process
        virtual void ioConfigChanged(audio_io_config_event event,
                                     const sp<AudioIoDescriptor>& ioDesc);


        status_t addAudioDeviceCallback(const sp<AudioDeviceCallback>& callback,
                                               audio_io_handle_t audioIo);
        status_t removeAudioDeviceCallback(const sp<AudioDeviceCallback>& callback,
                                           audio_io_handle_t audioIo);

        audio_port_handle_t getDeviceIdForIo(audio_io_handle_t audioIo);

    private:
        Mutex                               mLock;
        DefaultKeyedVector<audio_io_handle_t, sp<AudioIoDescriptor> >   mIoDescriptors;
        DefaultKeyedVector<audio_io_handle_t, Vector < sp<AudioDeviceCallback> > >
                                                                        mAudioDeviceCallbacks;
        // cached values for recording getInputBufferSize() queries
        size_t                              mInBuffSize;    // zero indicates cache is invalid
        uint32_t                            mInSamplingRate;
        audio_format_t                      mInFormat;
        audio_channel_mask_t                mInChannelMask;
        sp<AudioIoDescriptor> getIoDescriptor_l(audio_io_handle_t ioHandle);
    };

    class AudioPolicyServiceClient: public IBinder::DeathRecipient,
                                    public BnAudioPolicyServiceClient
    {
    public:
        AudioPolicyServiceClient() {
        }

        int addAudioPortCallback(const sp<AudioPortCallback>& callback);
        int removeAudioPortCallback(const sp<AudioPortCallback>& callback);

        // DeathRecipient
        virtual void binderDied(const wp<IBinder>& who);

        // IAudioPolicyServiceClient
        virtual void onAudioPortListUpdate();
        virtual void onAudioPatchListUpdate();
        virtual void onDynamicPolicyMixStateUpdate(String8 regId, int32_t state);
        virtual void onRecordingConfigurationUpdate(int event, audio_session_t session,
                        audio_source_t source, const audio_config_base_t *clientConfig,
                        const audio_config_base_t *deviceConfig, audio_patch_handle_t patchHandle);
        virtual void onOutputSessionEffectsUpdate(sp<AudioSessionInfo>& info, bool added);

    private:
        Mutex                               mLock;
        Vector <sp <AudioPortCallback> >    mAudioPortCallbacks;
    };

    static const sp<AudioFlingerClient> getAudioFlingerClient();
    static sp<AudioIoDescriptor> getIoDescriptor(audio_io_handle_t ioHandle);

    static sp<AudioFlingerClient> gAudioFlingerClient;
    static sp<AudioPolicyServiceClient> gAudioPolicyServiceClient;
    friend class AudioFlingerClient;
    friend class AudioPolicyServiceClient;

    static Mutex gLock;      // protects gAudioFlinger and gAudioErrorCallback,
    static Mutex gLockAPS;   // protects gAudioPolicyService and gAudioPolicyServiceClient
    static sp<IAudioFlinger> gAudioFlinger;
    static audio_error_callback gAudioErrorCallback;
    static dynamic_policy_callback gDynPolicyCallback;
    static record_config_callback gRecordConfigCallback;
    static audio_session_callback gAudioSessionCallback;

    static size_t gInBuffSize;
    // previous parameters for recording buffer size queries
    static uint32_t gPrevInSamplingRate;
    static audio_format_t gPrevInFormat;
    static audio_channel_mask_t gPrevInChannelMask;

    static sp<IAudioPolicyService> gAudioPolicyService;
};

};  // namespace android

#endif  /*ANDROID_AUDIOSYSTEM_H_*/
