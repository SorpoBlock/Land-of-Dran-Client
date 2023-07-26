#include "interpolator.h"

namespace syj
{
    unsigned int getServerTime() { return duration_cast< milliseconds >(steady_clock::now().time_since_epoch()).count() % 1000000000; }

    unsigned int interpolator::clientTimePoint;
    unsigned int interpolator::serverTimePoint;
    bool interpolator::useTripleInterpolation = false;

    void interpolator::addTransform(unsigned int packetTime,glm::vec3 pos,glm::quat rotation,glm::vec3 vel)
    {
        unsigned int currentServerTime = (getServerTime() - interpolator::clientTimePoint) + interpolator::serverTimePoint;
        modelTransform tmp;
        tmp.position = pos;
        tmp.rotation = rotation;
        tmp.velocity = vel;
        tmp.packetTime = packetTime;

        if(keyFrames.size() == 0)
        {
            keyFrames.push_back(tmp);
            return;
        }

        if(packetTime <= highestProcessed)
        {
            std::cout<<"HIGHESTPROCESSED\n";
            return;
        }

        for(unsigned int a = 0; a<keyFrames.size(); a++)
        {
            if(packetTime == keyFrames[a].packetTime)
            {
                std::cout<<"DUPLICATE!"<<packetTime<<"\n";
                return;
            }
        }

        for(unsigned int a = 0; a<keyFrames.size(); a++)
        {
            if(packetTime < keyFrames[a].packetTime)
            {
                keyFrames.insert(keyFrames.begin()+a,tmp);
                return;
            }
        }

        keyFrames.push_back(tmp);
    }

    glm::vec3 interpolator::guessVelocity()
    {
        return glm::vec3(0,0,0);
    }

    float getProg(modelTransform &a,modelTransform &b)
    {
        unsigned int currentServerTime = (getServerTime() - interpolator::clientTimePoint) + interpolator::serverTimePoint;
        unsigned int totdiff = b.packetTime - a.packetTime;
        unsigned int progress = currentServerTime - a.packetTime;
        float res = ((double)progress) / ((double)totdiff);
        if(res < 0 || res > 1)
            std::cout<<"Progress: "<<res<<"\n";
        return res;
    }

    void interpolator::getFrames(std::vector<modelTransform> &p,float &progress)
    {
        modelTransform a,b,c;
        if(keyFrames.size() < 1)
        {
            a.position = glm::vec3(0,0,0);
            a.rotation = glm::quat(1,0,0,0);
            b = a;
            progress = 0;
            p.push_back(a);
            p.push_back(b);
            return;
        }

        if(keyFrames.size() == 1)
        {
            a = keyFrames[0];
            b = keyFrames[0];
            progress = 0;
            p.push_back(a);
            p.push_back(b);
            return;
        }

        unsigned int currentServerTime = (getServerTime() - interpolator::clientTimePoint) + interpolator::serverTimePoint;

        if(keyFrames[0].packetTime > currentServerTime)
        {
            //std::cout<<"Behind "<<keyFrames[0].packetTime<<"-"<<currentServerTime<<"="<<(keyFrames[0].packetTime - currentServerTime)<<"\n";
            if(fabs((keyFrames[0].packetTime - currentServerTime)) < 400)
                interpolator::serverTimePoint += (keyFrames[0].packetTime - currentServerTime);
            a = keyFrames[0];
            b = keyFrames[1];
            progress = getProg(a,b);
            p.push_back(a);
            p.push_back(b);
            return;
        }

        int idx = 1;
        if(interpolator::useTripleInterpolation)
            idx = 2;
        if(keyFrames[keyFrames.size() - idx].packetTime < currentServerTime)
        {
            //std::cout<<"Subtracting... "<<keyFrames[keyFrames.size() - idx].packetTime<<" < "<<currentServerTime<<"\n";
            if(keyFrames.size() > 2 && interpolator::useTripleInterpolation)
            {
                if(fabs((currentServerTime - keyFrames[keyFrames.size()-3].packetTime)) < 400)
                    interpolator::serverTimePoint -= (currentServerTime - keyFrames[keyFrames.size()-3].packetTime);
            }
            else
            {
                if(fabs((currentServerTime - keyFrames[keyFrames.size()-2].packetTime)) < 400)
                    interpolator::serverTimePoint -= (currentServerTime - keyFrames[keyFrames.size()-2].packetTime);
            }
            a = keyFrames[keyFrames.size()-2];
            b = keyFrames[keyFrames.size()-1];
            progress = getProg(a,b);
            p.push_back(a);
            p.push_back(b);
            return;
        }

        for(int i = 1; i<keyFrames.size(); i++)
        {
            if(keyFrames[i].packetTime > currentServerTime)
            {
                if(i + 1 <= keyFrames.size() && interpolator::useTripleInterpolation)
                {
                    a = keyFrames[i-1];
                    b = keyFrames[i];
                    c = keyFrames[i+1];
                    progress = getProg(a,c);
                    for(int z = 0; z<i-1; z++)
                        keyFrames.erase(keyFrames.begin());
                    p.push_back(a);
                    p.push_back(b);
                    p.push_back(c);
                    return;
                }
                else
                {
                    int oldSize = keyFrames.size();

                    a = keyFrames[i-1];
                    b = keyFrames[i];
                    progress = getProg(a,b);
                    for(int z = 0; z<i-1; z++)
                        keyFrames.erase(keyFrames.begin());
                    p.push_back(a);
                    p.push_back(b);

                    if(i < oldSize-2 && keyFrames.size() > 2)
                        interpolator::serverTimePoint += 1;

                    return;
                }
            }
        }
    }

    glm::vec3 interpolator::getPosition()
    {
        float progress;
        std::vector<modelTransform> p;
        getFrames(p,progress);

        if(p.size() == 3)
        {
            lastPositionReturned = glm::vec3((1.0 - progress) * (1.0 - progress)) * p[0].position + glm::vec3(2.0 * progress * (1.0 - progress)) * p[1].position + glm::vec3(progress * progress) * p[2].position;
            //std::cout<<p[0].position.x<<","<<p[1].position.x<<","<<p[2].position.x<<" progress = "<<ret.x<<"\n";
            return lastPositionReturned;
        }
        else if(p.size() == 2)
        {
            lastPositionReturned = glm::mix(p[0].position,p[1].position,progress);
            return lastPositionReturned;
        }
        else
        {
            //std::cout<<"Size was " << std::to_string(p.size()) <<"\n";
            return lastPositionReturned;
        }
    }

    glm::quat interpolator::getRotation()
    {
        float progress;
        std::vector<modelTransform> p;
        getFrames(p,progress);

        if(p.size() == 3)
        {
            lastQuatReturned = glm::slerp(p[0].rotation,p[2].rotation,progress);
            return lastQuatReturned;
        }
        else if(p.size() == 2)
        {
            lastQuatReturned = glm::slerp(p[0].rotation,p[1].rotation,progress);
            return lastQuatReturned;
        }
        else
            return lastQuatReturned;
            //std::cout<<"Size was " << std::to_string(p.size()) <<"\n";
    }

    void interpolator::advance(float deltaMS)
    {

    }
}






