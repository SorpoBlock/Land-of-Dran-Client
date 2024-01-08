#ifndef AVATARPICKER_H_INCLUDED
#define AVATARPICKER_H_INCLUDED

#include "code/networking/client.h"
#include "code/graphics/camera.h"
#include "code/graphics/renderContext.h"
#include "code/graphics/renderTarget.h"
#include "code/graphics/uniformsBasic.h"
#include "code/graphics/newModel.h"
#include "code/utility/ceguiHelper.h"
#include "code/utility/preference.h"

namespace syj
{
    struct avatarPicker
    {
        CEGUI::Window *colorPicker = 0;
        CEGUI::Window *buttons = 0;
        CEGUI::Window *decalPicker = 0;
        void addDecalToPicker(std::string fileName);
        std::vector<texture*> faceDecals;
        std::vector<std::string> faceDecalFilepaths;
        std::vector<glm::vec3> nodeColors;
        std::vector<glm::vec3> preNodeColors;
        int chosenDecal = 0;
        int preChosenDecal = 0;
        perspectiveCamera pickingCamera;
        bool picking = false;
        int settingColorFor = -1;
        //model *playerModel = 0;
        newDynamic *pickingPlayer = 0;
        client *tmpClient = 0;
        preferenceFile *tmpPrefs = 0;
        renderTarget *pickingTexture = 0;
        bool sliderEventProtection = false;
        void runPickCycle(renderContext *context,uniformsHolder *instancedUnis,uniformsHolder *nonInstancedUnis,client *connection,preferenceFile *prefs);
        float modelYaw = 0;
        float modelPitch = 0;

        void loadFromPrefs(preferenceFile *prefs);
        avatarPicker();
        void sendAvatarPrefs(client *connection,preferenceFile *prefs);
    };
}

#endif // AVATARPICKER_H_INCLUDED
