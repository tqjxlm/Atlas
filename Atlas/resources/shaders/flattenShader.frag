#version 330 core
uniform vec2 boundary[16];  
uniform float nVert;  
uniform sampler2D tex; 
in vec2 texCoords; 
in vec3 fragPos;
 
bool pnpoly(vec2 test) 
{ 
    bool c = false; 
    if (nVert >= 3) 
    { 
        int i, j; 
        for (i = 0, j = int(nVert) - 1; i < int(nVert); j = i++) { 
            if (((boundary[i].y>test.y) != (boundary[j].y>test.y)) && 
                (test.x < (boundary[j].x - boundary[i].x) * (test.y - boundary[i].y) / (boundary[j].y - boundary[i].y) + boundary[i].x)) 
                c = !c; 
        } 
    } 
    return c; 
} 
 
bool is_hole(vec3 pos) 
{ 
    return pnpoly(pos.xy); 
} 
 
void main(void)  
{  
     
    if (is_hole(fragPos.xyz))
        discard; 
    else 
        gl_FragColor = texture2D(tex, texCoords);
	"}