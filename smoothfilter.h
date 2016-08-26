#ifndef SMOOTHFILTER_H
#define SMOOTHFILTER_H

/*
Schema of common filter:
            --------------------
sample ---->|  filter[length]  |-----> outputValue
            --------------------
Array filter store inputed samples multiplied by smoothFactor.

newSample = sample * smoothFactor;
outputValue = outputValue + newSample - oldestSample;
filter[currentPos++] = newSample;

New sample (multiplied by smoothFactor) overwrite oldest in array - ring addressing used.
*/

class SmoothFilter
{
public:
    SmoothFilter();
    SmoothFilter(int length, double smoothFactor);
    ~SmoothFilter();
    
    double processSample(double sample);
    int getLength() const {
        return length;
    }
    
private:
    double *filter;
    int length;
    double smoothFactor;
    int globalPos; // store count of processed samples
    double outputValue;
};

#endif // SMOOTHFILTER_H
