/* UI-thread requests consumed by the game thread. No guest pointers or
command strings cross the Android host/guest boundary. */
#include <jni.h>
#include <pthread.h>

#define CHEAT_QUEUE_CAPACITY 8
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int state, requests[CHEAT_QUEUE_CAPACITY], first, count;

void host_android_cheats_state(int value)
{
	pthread_mutex_lock(&lock);
	state = value;
	if (!(value & 2)) first = count = 0;
	pthread_mutex_unlock(&lock);
}

int host_android_cheats_read(void)
{
	int result = -1;
	pthread_mutex_lock(&lock);
	if ((state & 2) && count)
	{
		result = requests[first];
		first = (first + 1) % CHEAT_QUEUE_CAPACITY;
		count--;
	}
	pthread_mutex_unlock(&lock);
	return result;
}

JNIEXPORT jint JNICALL Java_com_halo_decomp_AndroidCheatMenu_nativeState(JNIEnv *env, jclass cls)
{
	int result;
	(void)env;
	(void)cls;
	pthread_mutex_lock(&lock);
	result = state;
	pthread_mutex_unlock(&lock);
	return result;
}

JNIEXPORT jboolean JNICALL Java_com_halo_decomp_AndroidCheatMenu_nativeRequest(JNIEnv *env, jclass cls, jint request)
{
	int accepted = 0;
	(void)env;
	(void)cls;
	pthread_mutex_lock(&lock);
	if ((state & 2) && count < CHEAT_QUEUE_CAPACITY && (request & ~0x10F) == 0 &&
		(request & 0xFF) <= 12 && ((request & 0xFF) < 9 || !(request & 0x100)))
	{
		requests[(first + count) % CHEAT_QUEUE_CAPACITY] = request;
		count++;
		accepted = 1;
	}
	pthread_mutex_unlock(&lock);
	return accepted ? JNI_TRUE : JNI_FALSE;
}
