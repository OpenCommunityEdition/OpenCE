#include "../port/mac/mac_glsl120.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); } } while (0)

int main(void)
{
    static const char vertex[]=
        "#version 450 core\n"
        "uniform vec4 c[192];\n"
        "out vec4 xD0;\n"
        "out vec3 xWorldPosition;\n"
        "invariant gl_Position;\n"
        "vec4 unpack_normpacked3(uint p)\n{\n return vec4(0.0);\n}\n"
        "layout(location = 0) in vec4 v0_in;\n"
        "layout(location = 1) in vec4 v1_in;\n"
        "layout(location = 10) in vec4 v10_in;\n"
        "void main()\n{\n\tvec4 v0 = v0_in;\n\tvec4 v1 = v1_in;\n\tvec4 v10 = v10_in;\n"
        "\tvec4 oPos = v0 + v10;\n\tgl_Position = oPos;\n\txD0 = vec4(1.0);\n"
        "\txWorldPosition = oPos.xyz;\n}\n";
    static const char extra_input_use[]=
        "#version 450 core\n"
        "layout(location = 0) in vec4 v0_in;\n"
        "layout(location = 1) in vec4 v1_in;\n"
        "void main() {\n\tvec4 v0 = v0_in;\n\tvec4 v1 = v1_in;\n"
        " gl_Position = v0;\n xD0 = v1_in;\n}\n";
    static const char changed_initializer[]=
        "#version 450 core\n"
        "layout(location = 0) in vec4 v0_in;\n"
        "layout(location = 1) in vec4 v1_in;\n"
        "void main() {\n\tvec4 v0 = v0_in;\n\tvec4 v1= v1_in;\n"
        " gl_Position = v0;\n xD0 = vec4(1.0);\n}\n";
    static const char pixel[]=
        "#version 450 core\n"
        "in vec4 xD0;\n"
        "in vec3 xWorldPosition;\n"
        "layout(location = 0) out vec4 fragment_color;\n"
        "uniform vec4 fog_color;\n"
        "void main()\n{\n fragment_color = clamp(xD0 + vec4(xWorldPosition, 0.0), 0.0, 1.0);\n}\n";
    static const char unsupported_interface[]=
        "#version 450 core\n"
        "out ivec3 unsupported_value;\n"
        "layout(location = 0) in vec4 v0_in;\n"
        "void main() { gl_Position = v0_in; }\n";
    static const char packed[]=
        "#version 450 core\n"
        "layout(location = 0) in uint v0_packed;\n";
    static const char textured[]=
        "#version 450 core\n"
        "layout(location = 0) out vec4 fragment_color;\n"
        "uniform sampler2D tex0;\n"
        "void main() { fragment_color = texture(tex0, vec2(0.0)); }\n";
    static const char wrong_stage[]=
        "#version 450 core\n"
        "layout(location = 0) out vec4 fragment_color;\n"
        "uniform sampler2D tex4;\n"
        "void main() { fragment_color = texture(tex4, vec2(0.0)); }\n";
    static const char unsupported_texture_query[]=
        "#version 450 core\n"
        "layout(location = 0) out vec4 fragment_color;\n"
        "uniform sampler2D tex0;\n"
        "void main() { fragment_color = vec4(textureSize(tex0, 0), 0.0, 1.0); }\n";
    static const char relative_constants[]=
        "#version 450 core\n"
        "layout(location = 0) in vec4 v0_in;\n"
        "void main() { gl_Position = v0_in * c[clamp(a0, 0, 191)]; }\n";
    static const char bounded_relative[]=
        "#version 450 core\n"
        "layout(location = 0) in vec4 v0_in;\n"
        "void main() { gl_Position = v0_in * c[clamp(a0 + 255, 0, 191)]; }\n";
    static const char duplicate_position[]=
        "#version 450 core\n"
        "layout(location = 0) in vec4 v0_in;\n"
        "void main() { gl_Position = v0_in; gl_Position = v0_in; }\n";
    static const char multi[]=
        "#version 450 core\n"
        "layout(location = 0) out vec4 fragment_color;\n"
        "uniform sampler2D tex1;\n"
        "uniform sampler3D tex2;\n"
        "uniform samplerCube tex3;\n"
        "void main() { fragment_color = texture(tex1, vec2(0.0)) + texture(tex2, vec3(0.0)) + texture(tex3, vec3(1.0)); }\n";
    static const char generated_relative[]=
        "#version 450 core\n"
        "layout(location = 0) in vec4 v0_in;\n"
        "uniform vec4 c[192];\n"
        "void main() {\n"
        "\tint a0 = 0;\n"
        "\tvec4 mac = vec4(3.0);\n"
        "\ta0 = int(floor(mac.x + 0.001));\n"
        "\tgl_Position = c[clamp(a0 + 60, 0, 191)];\n}\n";
    char error[128], *converted=NULL, unknown_address[1024];
    unsigned long attributes=0;
    int ok;
    ok=mac_glsl120_vertex(vertex,&converted,&attributes,error,sizeof(error));
    if(!ok)fprintf(stderr,"vertex conversion: %s\n",error);
    CHECK(ok && converted && attributes==1025);
    if (converted) {
        CHECK(strstr(converted,"#version 120\n")!=NULL);
        CHECK(strstr(converted,"attribute vec4 v0_in;")!=NULL);
        CHECK(strstr(converted,"attribute vec4 v1_in;")==NULL);
        CHECK(strstr(converted,"\tvec4 v1 = v1_in;")==NULL);
        CHECK(strstr(converted,"attribute vec4 v10_in;")!=NULL);
        CHECK(strstr(converted,"varying vec4 xD0;")!=NULL);
        CHECK(strstr(converted,"varying vec3 xWorldPosition;")!=NULL);
        CHECK(strstr(converted,"gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;")!=NULL);
        CHECK(strstr(converted,"uint") == NULL && strstr(converted,"layout(")==NULL);
        free(converted); converted=NULL;
    }
    CHECK(mac_glsl120_vertex(extra_input_use,&converted,&attributes,error,sizeof(error)));
    CHECK(converted && (attributes&2UL)!=0);
    CHECK(converted && strstr(converted,"attribute vec4 v1_in;")!=NULL);
    CHECK(converted && strstr(converted,"\tvec4 v1 = v1_in;")!=NULL);
    free(converted); converted=NULL;
    CHECK(mac_glsl120_vertex(changed_initializer,&converted,&attributes,error,sizeof(error)));
    CHECK(converted && (attributes&2UL)!=0);
    CHECK(converted && strstr(converted,"attribute vec4 v1_in;")!=NULL);
    CHECK(converted && strstr(converted,"\tvec4 v1= v1_in;")!=NULL);
    free(converted); converted=NULL;
    CHECK(!mac_glsl120_vertex(packed,&converted,&attributes,error,sizeof(error)));
    CHECK(!converted);
    CHECK(!mac_glsl120_vertex(unsupported_interface,&converted,&attributes,error,sizeof(error)));
    CHECK(!converted);
    CHECK(!mac_glsl120_vertex(relative_constants,&converted,&attributes,error,sizeof(error)));
    CHECK(!converted);
    CHECK(!mac_glsl120_vertex(duplicate_position,&converted,&attributes,error,sizeof(error)));
    CHECK(!converted);
    CHECK(mac_glsl120_vertex(bounded_relative,&converted,&attributes,error,sizeof(error)));
    CHECK(converted && strstr(converted,"c[int(clamp(float(a0) + 255.0, 0.0, 191.0))]"));
    free(converted); converted=NULL;
    CHECK(mac_glsl120_vertex(generated_relative,&converted,&attributes,error,sizeof(error)));
    CHECK(converted && strstr(converted,"float a0 = 0.0;"));
    CHECK(converted && strstr(converted,"a0 = floor(mac.x + 0.001);"));
    CHECK(converted && strstr(converted,"c[int(clamp(float(a0) + 60.0, 0.0, 191.0))]"));
    free(converted); converted=NULL;
    strcpy(unknown_address,generated_relative);
    /* An unrecognized use must prevent address-register promotion. */
    strcat(unknown_address,"// a0 unknown use\n");
    CHECK(mac_glsl120_vertex(unknown_address,&converted,&attributes,error,sizeof(error)));
    CHECK(converted && strstr(converted,"int a0 = 0;"));
    CHECK(converted && strstr(converted,"a0 = int(floor(mac.x + 0.001));"));
    free(converted); converted=NULL;
    ok=mac_glsl120_pixel(pixel,&converted,error,sizeof(error));
    if(!ok)fprintf(stderr,"pixel conversion: %s\n",error);
    CHECK(ok && converted);
    if(converted){
        CHECK(strstr(converted,"#version 120\n")!=NULL);
        CHECK(strstr(converted,"varying vec4 xD0;")!=NULL);
        CHECK(strstr(converted,"varying vec3 xWorldPosition;")!=NULL);
        CHECK(strstr(converted,"gl_FragColor = clamp")!=NULL);
        CHECK(strstr(converted,"fragment_color")==NULL);
        free(converted); converted=NULL;
    }
    CHECK(mac_glsl120_pixel(textured,&converted,error,sizeof(error)));
    CHECK(converted && strstr(converted,"texture2D(tex0,")!=NULL);
    free(converted); converted=NULL;
    CHECK(mac_glsl120_pixel(multi,&converted,error,sizeof(error)));
    CHECK(converted && strstr(converted,"texture2D(tex1,") && strstr(converted,"texture3D(tex2,") && strstr(converted,"textureCube(tex3,"));
    free(converted); converted=NULL;
    CHECK(!mac_glsl120_pixel(wrong_stage,&converted,error,sizeof(error)));
    CHECK(!converted);
    CHECK(!mac_glsl120_pixel(unsupported_texture_query,&converted,error,sizeof(error)));
    CHECK(!converted);
    printf("mac_glsl120 checks=%d failures=%d\n",checks,failures);
    return failures?1:0;
}
