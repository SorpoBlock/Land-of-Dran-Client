#ifndef BRICKSTRUCTURE_H_INCLUDED
#define BRICKSTRUCTURE_H_INCLUDED

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

namespace syj
{
    enum brickMaterial{none=0,tempBrickEffect=1,undulo=1000,bob=2000,peral=2,chrome=3,glow=4,blink=5,swirl=6,rainbow=7,slippery=8,foil=9};

    //Holds information that applies to every single brick:
    struct brickRenderData
    {
        //Needed to avoid crash with clearAllBricks packet
        bool isTempBrick = false;

        //For cars:
        int carPlatesUp = 0;
        bool yHalfPos = false;

        //Only used for loading brick cars atm:
        int figuredAngleID = 0;

        bool placedInTransparentBuffer = false;
        bool markedForDeath = false;
        int serverID = -1;
        btRigidBody *body = 0;
        //This will be changed first, before body is created or removed
        bool shouldCollide = true;
        int material = none;
        glm::vec3 position;
        //Warning, this isn't actually a quaternion. It's rotation of W radians around XYZ direction.
        glm::quat rotation;
        glm::vec4 color;
    };

    enum faceDirection{FACE_UP=0,FACE_DOWN=1,FACE_NORTH=2,FACE_SOUTH=3,FACE_EAST=4,FACE_WEST=5,FACE_OMNI=6};
    enum defaultBrickTextures{BRICKTEX_STUDS=0,BRICKTEX_SIDES=1,BRICKTEX_BOTTOM=2,BRICKTEX_RAMP=3,BRICKTEX_PRINT=4};
    enum newBrickShaderLayout{BRICKLAYOUT_VERTS=0,BRICKLAYOUT_POSITIONMAT=1,BRICKLAYOUT_ROTATION=2,
                            BRICKLAYOUT_PAINTCOLOR=3,BRICKLAYOUT_DIMENSION=4,BRICKLAYOUT_DIRECTION=5,BRICKLAYOUT_NORMAL=6,BRICKLAYOUT_UV=7,BRICKLAYOUT_VERTEXCOLOR=8};
    enum perTextureBuffers{positionMatBuffer=0,rotationBuffer=1,paintColorBuffer=2,dimensionBuffer=3,directionBuffer=4,customNormalBuffer=5,customUVBuffer=6,customVertexColorBuffer=7,customVertexBuffer=8};
}

#endif // BRICKSTRUCTURE_H_INCLUDED
