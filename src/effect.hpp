//spencer jackson
//gpl v3 bla bla
#include<stdint.h>
#include<stdlib.h>

//this is an abstract class defining the interface that every effect must implement
class effect
{
    //effect specific dsp processing
    void process(uint32_t nframes, float** input, float** output)
    {
        if(bypassState==BYPASS_ON)
            passthrough(nframes, input, output);
        else
            processDSP(nframes, input, output);
    }

    //effect specific parameter setting
    virtual bool setParameter(uint8_t id, float value) = 0;
    
    //check if effect is bypassed
    bool isBypassed() {return bypassState==BYPASS_ON;}

    //function to call to bypass effect 
    //bypass will be fully enabled after process() goes through xfadeLength samples
    void setBypass(bool bypass)
    {
        //TODO: This could cause some weirdness if called outside of the realtime process thread
        //we may need to put more effort into what happens if toggled while already in a transition
        if(bypass && bypassState == BYPASS_OFF)
        {
            bypass = xfadeDirection = BYPASS_TURNING_ON;
            xfadeStepCount =  0;
        }
        else if(!bypass && bypassState == BYPASS_ON)
        {
            activate();
            bypass = xfadeDirection = BYPASS_TURNING_OFF;
            xfadeStepCount =  0;
        }
    }

    //TODO: may want to implement parameter smoothing for mix
    float wetMix = 1.0; //!< coefficient for wet to dry mix

private:
    //effect specific DSP code
    virtual bool processDSP(uint32_t nframes, float** input, float** output) = 0;
    
    //effect specific setup after bypass, before process
    virtual bool activate() = 0;
    
    //this function is to be used by every effect to copy each sample to the output buffer
    float mix(uint32_t frame, float* wet, float** input, float** output)
    {
        //TODO: probably want the mix style from the original rather than just this linear blend
        const float wmix =  wetMix*xfadeValue;
        const float dmix =  1.f-wmix;
        for(int chan=0; chan<outputBufferCount; chan++)
            output[chan][frame] = wmix*wet[chan] + dmix*input[chan][frame];
        if(xfadeCount<=xfadeLength)
        {
            xfadeValue = 0.5 + xfadeDirection*(0.5 - xfadeCount++/(float)xfadeLength);
        }
    }

    int8_t bypassState;
    static const int8_t BYPASS_OFF = 0;
    static const int8_t BYPASS_TURNING_OFF = 1;
    static const int8_t BYPASS_ON = -2;
    static const int8_t BYPASS_TURNING_ON = -1;

    uint8_t inputBufferCount;
    uint8_t outputBufferCount;

    uint16_t xfadeLength;
    uint16_t xfadeCount = 0;
    int8_t xfadeDirection = BYPASS_TURNING_OFF;
    float xfadeValue = 0.0;


    float passthrough(uint32_t nframes, float** input, float** output)
    {
        for(int chan=0; chan<outputBufferCount; chan++)
            if(output[chan] != input[chan])
                memcpy(output[chan], input[chan], nframes*sizeof(float));
        
    }
}
