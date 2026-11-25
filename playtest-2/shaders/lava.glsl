#version 330

// Colours the lava by how hot it is. Every mistake raises the heat by one.
//   heat 0: the lava's own orange
//   heat 1: deep red
//   heat 2: violet

uniform sampler2D texture0;
uniform int heat;

in vec2 fragTexCoord;
in vec2 fragPosition;

out vec4 finalColor;

void main()
{
    vec4 color = texture(texture0, fragTexCoord);
    vec3 tint  = color.rgb;

    if (heat == 1)
    {
        tint = vec3(color.r, color.g * 0.35, color.b * 0.3);
    }
    else if (heat == 2)
    {
        float brightness = (color.r + color.g + color.b) / 3.0;
        tint = vec3(brightness * 1.3 + 0.15,
                    brightness * brightness * 0.9,
                    brightness * 1.2 + 0.2);
    }

    finalColor = vec4(tint, color.a);
}
