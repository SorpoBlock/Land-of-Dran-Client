#include "fontRender.h"

namespace syj
{
    fontRenderer::fontRenderer(FT_Library &ft,std::string filename,GLenum wrap,GLenum minFilter,GLenum magFilter)
    {
        scope("fontRenderer::fontRenderer");

        if (FT_New_Face(ft, filename.c_str(), 0, &face))
        {
            error("FREETYPE: Failed to load font");
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, 48);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for(unsigned char c = 0; c<128; c++)
        {
            if(!FT_Load_Char(face,c,FT_LOAD_RENDER))
            {
                glGenTextures(1,&glyphs[c].texture);
                glBindTexture(GL_TEXTURE_2D,glyphs[c].texture);
                glTexImage2D(GL_TEXTURE_2D,0,GL_RED,face->glyph->bitmap.width,face->glyph->bitmap.rows,0,GL_RED,GL_UNSIGNED_BYTE,face->glyph->bitmap.buffer);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

                glyphs[c].size = glm::ivec2(face->glyph->bitmap.width,face->glyph->bitmap.rows);
                glyphs[c].bearing = glm::ivec2(face->glyph->bitmap_left,face->glyph->bitmap_top);
                glyphs[c].advance = face->glyph->advance.x;
                glyphs[c].used = true;
            }
        }

        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        glBindTexture(GL_TEXTURE_2D,0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        glGenVertexArrays(1, &squareVAO);
        glGenBuffers(1, &squareVBO);
        glBindVertexArray(squareVAO);
        glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void fontRenderer::naiveRender(uniformsHolder &unis,std::string text,glm::vec3 offset,float scale,glm::vec3 color)
    {
        const char *t = text.c_str();
        if(!t)
            return;

        glUniform3f(unis.target->getUniformLocation("textColor"),color.r,color.g,color.b);
        glUniform3f(unis.target->getUniformLocation("textOffset"),offset.x,offset.y,offset.z);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(squareVAO);

        float negAdvance = 0;
        for(unsigned int c = 0; t[c]; c++)
        {
            if(t[c] >= 128)
                continue;
            if(t[c] < 0)
                continue;

            fontChar *ch = &glyphs[t[c]];

            if(!ch)
                continue;
            if(!ch->used)
                continue;

            negAdvance += (ch->advance >> 6) * scale;
        }


        // iterate through all characters
        glm::vec3 position = glm::vec3(0,0,0);
        for(unsigned int c = 0; t[c]; c++)
        {
            if(t[c] >= 128)
                continue;
            if(t[c] < 0)
                continue;

            fontChar *ch = &glyphs[t[c]];

            if(!ch)
                continue;
            if(!ch->used)
                continue;

            float xpos = position.x + ch->bearing.x * scale - negAdvance / 2.0;
            float ypos = position.y - (ch->size.y - ch->bearing.y) * scale;

            float w = ch->size.x * scale;
            float h = ch->size.y * scale;
            // update VBO for each character
            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };
            // render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch->texture);
            // update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            // render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            position.x += (ch->advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
