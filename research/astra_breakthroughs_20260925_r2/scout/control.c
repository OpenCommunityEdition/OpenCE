/* Independent, typed-member compiler mechanism control. Not game source. */
typedef struct pair { float x, y; } pair;
typedef struct triple { pair xy; float z; } triple;
extern void use2(pair *value);
extern void use3(triple *value);
extern void use1(float value);

void frame_control(float const *a, int mode)
{
    if (mode & 1)
    {
        pair alpha = {0.0f, 0.0f};
        triple left;
        pair horizontal;
        left.xy.x = a[0]*a[3]-a[1]*a[2];
        left.xy.y = a[1]*a[4]-a[2]*a[0];
        left.z = a[0]*a[5]-a[4]*a[3];
        horizontal.x = left.xy.x;
        horizontal.y = left.xy.y;
        use2(&horizontal);
        use2(&alpha);
        use3(&left);
    }
    if (mode & 2)
    {
        triple forward;
        forward.xy.x = a[0]*a[3];
        forward.xy.y = a[1]*a[4];
        forward.z = a[2]*a[5];
        use3(&forward);
        use1(forward.z);
    }
}
