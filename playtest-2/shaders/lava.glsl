#version 330

// Colours the lava by how hot it is. Every mistake raises the heat by one.
//   heat 0: the lava's own orange
//   heat 1: deep red
//   heat 2: violet plasma, rippling fast

uniform sampler2D texture0;
uniform int   heat;
uniform float time;

in vec2 fragTexCoord;
in vec2 fragPosition;

out vec4 finalColor;

void main()
{
    vec4 color = texture(texture0, fragTexCoord);

    // a slow ripple across the surface; it speeds up as the heat rises
    float rippleSpeed = 2.0 + float(heat) * 3.0;
    float ripple      = 0.88 + 0.12 * sin(time * rippleSpeed +
                                          fragPosition.x * 0.04 +
                                          fragPosition.y * 0.03);

    vec3 tint;

    if (heat == 0)
    {
        tint = color.rgb;
    }
    else if (heat == 1)
    {
        tint = vec3(color.r, color.g * 0.35, color.b * 0.3);
    }
    else
    {
        float brightness = (color.r + color.g + color.b) / 3.0;
        tint = vec3(brightness * 1.3 + 0.15,
                    brightness * brightness * 0.9,
                    brightness * 1.2 + 0.2);
    }

    finalColor = vec4(tint * ripple, color.a);
}
