#include "wrenchDialog.h"

namespace syj
{
    bool closeSteeringWrench(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *wrench = root->getChild("HUD/SteeringWrench");
        wrench->setVisible(false);
        serverStuff *ohWow = (serverStuff*)wrench->getUserData();
        ohWow->context->setMouseLock(true);

        return true;
    }

    bool closeWheelWrench(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *wrench = root->getChild("HUD/WheelWrench");
        wrench->setVisible(false);
        serverStuff *ohWow = (serverStuff*)wrench->getUserData();
        ohWow->context->setMouseLock(true);

        return true;
    }

    bool closeWrench(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *wrench = root->getChild("HUD/Wrench");
        wrench->setVisible(false);
        serverStuff *ohWow = (serverStuff*)wrench->getUserData();
        ohWow->context->setMouseLock(true);

        return true;
    }

    bool renderAllButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *wrench = root->getChild("HUD/Wrench");
        ((CEGUI::ToggleButton*)wrench->getChild("RenderNorth"))->setSelected(true);
        ((CEGUI::ToggleButton*)wrench->getChild("RenderSouth"))->setSelected(true);
        ((CEGUI::ToggleButton*)wrench->getChild("RenderUp"))->setSelected(true);
        ((CEGUI::ToggleButton*)wrench->getChild("RenderDown"))->setSelected(true);
        ((CEGUI::ToggleButton*)wrench->getChild("RenderEast"))->setSelected(true);
        ((CEGUI::ToggleButton*)wrench->getChild("RenderWest"))->setSelected(true);
        return true;
    }

    bool renderNoneButton(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *wrench = root->getChild("HUD/Wrench");
        ((CEGUI::ToggleButton*)wrench->getChild("RenderNorth"))->setSelected(false);
        ((CEGUI::ToggleButton*)wrench->getChild("RenderSouth"))->setSelected(false);
        ((CEGUI::ToggleButton*)wrench->getChild("RenderUp"))->setSelected(false);
        ((CEGUI::ToggleButton*)wrench->getChild("RenderDown"))->setSelected(false);
        ((CEGUI::ToggleButton*)wrench->getChild("RenderEast"))->setSelected(false);
        ((CEGUI::ToggleButton*)wrench->getChild("RenderWest"))->setSelected(false);
        return true;
    }

    bool pitchSliderMoved(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *wrench = root->getChild("HUD/Wrench");

        CEGUI::Slider *pitchSlider = (CEGUI::Slider*)wrench->getChild("PitchSlider");
        if(pitchSlider->getCurrentValue() < 25)
            pitchSlider->setCurrentValue(25);
        if(pitchSlider->getCurrentValue() > 400)
            pitchSlider->setCurrentValue(400);

        wrench->getChild("PitchResult")->setText(std::to_string(pitchSlider->getCurrentValue()) + "%");

        return true;
    }

    bool submitWrench(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *wrench = root->getChild("HUD/Wrench");
        serverStuff *ohWow = (serverStuff*)wrench->getUserData();

        packet data;
        data.writeUInt(10,4);
        data.writeBit(false);
        data.writeBit(false);
        data.writeBit(((CEGUI::ToggleButton*)wrench->getChild("Colliding"))->isSelected());
        data.writeBit(!((CEGUI::ToggleButton*)wrench->getChild("RenderUp"))->isSelected());
        data.writeBit(!((CEGUI::ToggleButton*)wrench->getChild("RenderDown"))->isSelected());
        data.writeBit(!((CEGUI::ToggleButton*)wrench->getChild("RenderNorth"))->isSelected());
        data.writeBit(!((CEGUI::ToggleButton*)wrench->getChild("RenderSouth"))->isSelected());
        data.writeBit(!((CEGUI::ToggleButton*)wrench->getChild("RenderEast"))->isSelected());
        data.writeBit(!((CEGUI::ToggleButton*)wrench->getChild("RenderWest"))->isSelected());
        if(((CEGUI::Combobox*)wrench->getChild("MusicDropdown"))->getSelectedItem())
            data.writeUInt(((CEGUI::Combobox*)wrench->getChild("MusicDropdown"))->getSelectedItem()->getID(),10);
        else
            data.writeUInt(0,10);

        float pitch = ((CEGUI::Slider*)wrench->getChild("PitchSlider"))->getCurrentValue();
        pitch /= 100.0;
        data.writeFloat(pitch);
        data.writeString(wrench->getChild("BrickName")->getText().c_str());

        if(((CEGUI::ToggleButton*)wrench->getChild("UseLight"))->isSelected())
        {
            data.writeBit(true);
            data.writeFloat(atof(wrench->getChild("Red")->getText().c_str()));
            data.writeFloat(atof(wrench->getChild("Green")->getText().c_str()));
            data.writeFloat(atof(wrench->getChild("Blue")->getText().c_str()));
            if(((CEGUI::ToggleButton*)wrench->getChild("IsSpotlight"))->isSelected())
            {
                data.writeBit(true);
                data.writeFloat(((CEGUI::Slider*)wrench->getChild("PhiSlider"))->getCurrentValue());
                data.writeFloat(((CEGUI::Slider*)wrench->getChild("YawSlider"))->getCurrentValue());
                data.writeFloat(((CEGUI::Slider*)wrench->getChild("LightPitchSlider"))->getCurrentValue());
            }
            else
                data.writeBit(false);
        }
        else
            data.writeBit(false);

        ohWow->context->setMouseLock(true);
        ohWow->connection->send(&data,true);

        wrench->setVisible(false);
        return true;
    }

    bool submitWheelWrench(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *wheelWrench = root->getChild("HUD/WheelWrench");
        serverStuff *ohWow = (serverStuff*)wheelWrench->getUserData();

        packet data;
        data.writeUInt(10,4);
        data.writeBit(true);

        data.writeFloat(atof(wheelWrench->getChild("BreakForce")->getText().c_str()));
        data.writeFloat(atof(wheelWrench->getChild("SteeringForce")->getText().c_str()));
        data.writeFloat(atof(wheelWrench->getChild("EngineForce")->getText().c_str()));
        data.writeFloat(atof(wheelWrench->getChild("SuspensionLength")->getText().c_str()));
        data.writeFloat(atof(wheelWrench->getChild("SuspensionStiffness")->getText().c_str()));
        data.writeFloat(atof(wheelWrench->getChild("Friction")->getText().c_str()));
        data.writeFloat(atof(wheelWrench->getChild("RollInfluence")->getText().c_str()));
        data.writeFloat(atof(wheelWrench->getChild("DampingCompression")->getText().c_str()));
        data.writeFloat(atof(wheelWrench->getChild("DampingRelaxation")->getText().c_str()));

        ohWow->context->setMouseLock(true);
        ohWow->connection->send(&data,true);
        wheelWrench->setVisible(false);

        return true;
    }

    bool submitSteeringWrench(const CEGUI::EventArgs &e)
    {
        CEGUI::Window *root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
        CEGUI::Window *wheelWrench = root->getChild("HUD/SteeringWrench");
        serverStuff *ohWow = (serverStuff*)wheelWrench->getUserData();

        packet data;
        data.writeUInt(10,4);
        data.writeBit(false);
        data.writeBit(true);

        data.writeFloat(atof(wheelWrench->getChild("Mass")->getText().c_str()));
        data.writeFloat(atof(wheelWrench->getChild("Damping")->getText().c_str()));
        data.writeBit(((CEGUI::ToggleButton*)wheelWrench->getChild("CenterMass"))->isSelected());

        ohWow->context->setMouseLock(true);
        ohWow->connection->send(&data,true);
        wheelWrench->setVisible(false);

        return true;
    }

    void setUpWrenchDialogs(CEGUI::Window *hud,serverStuff *ohWow)
    {
        CEGUI::Window *wrench = hud->getChild("Wrench");

        wrench->setUserData(ohWow);
        wrench->getChild("RenderAll")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&renderAllButton));
        wrench->getChild("RenderNone")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&renderNoneButton));
        wrench->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeWrench));
        wrench->getChild("Submit")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&submitWrench));
        wrench->getChild("PitchSlider")->subscribeEvent(CEGUI::Slider::EventValueChanged,CEGUI::Event::Subscriber(&pitchSliderMoved));

        CEGUI::Window *wheelWrench = hud->getChild("WheelWrench");

        wheelWrench->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeWheelWrench));
        wheelWrench->getChild("Submit")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&submitWheelWrench));
        wheelWrench->setUserData(ohWow);

        CEGUI::Window *steeringWrench = hud->getChild("SteeringWrench");

        steeringWrench->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,CEGUI::Event::Subscriber(&closeSteeringWrench));
        steeringWrench->getChild("Submit")->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&submitSteeringWrench));
        steeringWrench->setUserData(ohWow);
    }
}









