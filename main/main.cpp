/*

retrOdroid The Retro Tape Interface

Hacked together by the BACKOFFICE crew

Chrissy
Dr A
Hopefully Sad Ken!

*/

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_partition.h"
#include "driver/i2s.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "esp_ota_ops.h"

extern "C"
{
#include "../components/odroid/odroid_settings.h"
#include "../components/odroid/odroid_audio.h"
#include "../components/odroid/odroid_input.h"
#include "../components/odroid/odroid_system.h"
#include "../components/odroid/odroid_display.h"
#include "../components/odroid/odroid_sdcard.h"

#include "../components/ugui/ugui.h"
}

#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define ESP32_PSRAM (0x3f800000)
const char* SD_BASE_PATH = "/sd";

#define AUDIO_SAMPLE_RATE (44100)

QueueHandle_t vidQueue;

#define RETRODROID_WIDTH 160
#define RETRODROID_HEIGHT 250
uint8_t framebuffer[RETRODROID_WIDTH * RETRODROID_HEIGHT];
uint16_t pal16[256];
bool IsPal;

static int AudioSink = ODROID_AUDIO_SINK_DAC;

#define I2S_SAMPLE_RATE   (44100)
#define SAMPLERATE I2S_SAMPLE_RATE // Sample rate of our waveforms in Hz

#define AMPLITUDE     1000
#define WAV_SIZE      256
int32_t sine[WAV_SIZE]     = {0};

void generateSine(int32_t amplitude, int32_t* buffer, uint16_t length) {
  // Generate a sine wave signal with the provided amplitude and store it in
  // the provided buffer of size length.
  for (int i=0; i<length; ++i) {
    buffer[i] = int32_t(float(amplitude)*sin(2.0*M_PI*(1.0/length)*i));
  }
}

void playWave2(int32_t* buffer, uint16_t length, float frequency, float seconds) {
  short outbuf[2];
  uint32_t iterations = seconds*SAMPLERATE;
  float delta = (frequency*length)/float(SAMPLERATE);
  odroid_audio_volume_set(ODROID_VOLUME_LEVEL4);
  for (uint32_t i=0; i<iterations; ++i) {
    uint16_t pos = uint32_t(i*delta) % length;
    int32_t sample = buffer[pos];
    outbuf[0] = sample;
    outbuf[1] = sample;
    odroid_audio_submit(outbuf, 1);
  }
  odroid_audio_volume_set(ODROID_VOLUME_LEVEL0);
  odroid_audio_submit(outbuf, 1);
  printf("DONE.\n");
}

void playWave(int amplitude, float frequency, float seconds) {
  short outbuf[2];
  int halfWavelength = (SAMPLERATE / frequency);
  uint32_t iterations = seconds*SAMPLERATE;
  int count = 0;

  odroid_audio_volume_set(ODROID_VOLUME_LEVEL4);
  short sample = amplitude;

  for (uint32_t i=0; i<iterations; ++i) {
    if (count % halfWavelength == 0) {
      // invert the sample every half wavelength count multiple to generate square wave
      sample = -1 * sample;
    }
    outbuf[0] = sample;
    outbuf[1] = sample;
    odroid_audio_submit(outbuf, 1);
    count++;
  }
  odroid_audio_volume_set(ODROID_VOLUME_LEVEL0);
  odroid_audio_submit(outbuf, 1);
  printf("DONE.\n");
}

void videoTask(void *arg)
{
    while(1)
    {
        uint8_t* param;
        xQueuePeek(vidQueue, &param, portMAX_DELAY);
        memcpy(framebuffer, param, sizeof(framebuffer));
        xQueueReceive(vidQueue, &param, portMAX_DELAY);

    }

    odroid_display_lock_sms_display();

    // Draw hourglass
    odroid_display_show_hourglass();

    odroid_display_unlock_sms_display();

    vTaskDelete(NULL);

    while (1) {}
}

// volatile bool test = true;
// volatile uint16_t test2 = true;

UG_GUI gui;
uint16_t* fb;

static void pset(UG_S16 x, UG_S16 y, UG_COLOR color)
{
    fb[y * 320 + x] = color;
}

static void window1callback(UG_MESSAGE* msg)
{
}

static void UpdateDisplay()
{
    UG_Update();
    ili9341_write_frame_rectangleLE(0, 0, 320, 240, fb);
}



// A utility function to swap two elements
inline static void swap(char** a, char** b)
{
    char* t = *a;
    *a = *b;
    *b = t;
}

static int strcicmp(char const *a, char const *b)
{
    for (;; a++, b++)
    {
        int d = tolower((int)*a) - tolower((int)*b);
        if (d != 0 || !*a) return d;
    }
}

//------
/* This function takes last element as pivot, places
   the pivot element at its correct position in sorted
    array, and places all smaller (smaller than pivot)
   to left of pivot and all greater elements to right
   of pivot */
static int partition (char* arr[], int low, int high)
{
    char* pivot = arr[high];    // pivot
    int i = (low - 1);  // Index of smaller element

    for (int j = low; j <= high- 1; j++)
    {
        // If current element is smaller than or
        // equal to pivot
        if (strcicmp(arr[j], pivot) < 0) //(arr[j] <= pivot)
        {
            i++;    // increment index of smaller element
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

/* The main function that implements QuickSort
 arr[] --> Array to be sorted,
  low  --> Starting index,
  high  --> Ending index */
static void quickSort(char* arr[], int low, int high)
{
    if (low < high)
    {
        /* pi is partitioning index, arr[p] is now
           at right place */
        int pi = partition(arr, low, high);

        // Separately sort elements before
        // partition and after partition
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

IRAM_ATTR static void bubble_sort(char** files, int count)
{
    int n = count;
    bool swapped = true;

    while (n > 0)
    {
        int newn = 0;
        for (int i = 1; i < n; ++i)
        {
            if (strcicmp(files[i - 1], files[i]) > 0)
            {
                char* temp = files[i - 1];
                files[i - 1] = files[i];
                files[i] = temp;

                newn = i;
            }
        } //end for
        n = newn;
    } //until n = 0
}

static void SortFiles(char** files, int count)
{
    int n = count;
    bool swapped = true;

    if (count > 1)
    {
        //quickSort(files, 0, count - 1);
        bubble_sort(files, count - 1);
    }
}

int GetFiles(const char* path, char*** filesOut)
{
    const int MAX_FILES = 4096;

    int count = 0;
    char** result = (char**)heap_caps_malloc(MAX_FILES * sizeof(void*), MALLOC_CAP_SPIRAM);
    if (!result) abort();

    DIR *dir = opendir(path);
    if( dir == NULL )
    {
        printf("opendir failed.\n");
        abort();
    }

    // Print files
    struct dirent *entry;
    while((entry=readdir(dir)) != NULL)
    {
        size_t len = strlen(entry->d_name);

        bool skip = false;

        // ignore 'hidden' files (MAC)
        if (entry->d_name[0] == '.') skip = true;

        // ignore BIOS file(s)
        char* lowercase = (char*)malloc(len + 1);
        if (!lowercase) abort();

        lowercase[len] = 0;
        for (int i = 0; i < len; ++i)
        {
            lowercase[i] = tolower((int)entry->d_name[i]);
        }
        if (strcmp(lowercase, "bios.col") == 0) skip = true;

        free(lowercase);


        if (!skip)
        {
                    result[count] = (char*)heap_caps_malloc(len + 1, MALLOC_CAP_SPIRAM);
                    //result[count] = (char*)malloc(len + 1);
                    if (!result[count])
                    {
                        abort();
                    }

                    strcpy(result[count], entry->d_name);
                    ++count;

                    if (count >= MAX_FILES) break;
        }
    }

    closedir(dir);

    SortFiles(result, count);

#if 0
    for (int i = 0; i < count; ++i)
    {
        printf("GetFiles: %d='%s'\n", i, result[i]);
    }
#endif

    *filesOut = result;
    return count;
}

void FreeFiles(char** files, int count)
{
    for (int i = 0; i < count; ++i)
    {
        free(files[i]);
    }

    free(files);
}





#define MAX_OBJECTS 20
#define ITEM_COUNT  10

UG_WINDOW window1;
UG_BUTTON button1;
UG_TEXTBOX textbox[ITEM_COUNT];
UG_OBJECT objbuffwnd1[MAX_OBJECTS];



void DrawPage(char** files, int fileCount, int currentItem)
{
    static const size_t MAX_DISPLAY_LENGTH = 38;

    int page = currentItem / ITEM_COUNT;
    page *= ITEM_COUNT;

    // Reset all text boxes
    for (int i = 0; i < ITEM_COUNT; ++i)
    {
        uint16_t id = TXB_ID_0 + i;
        //UG_TextboxSetForeColor(&window1, id, C_BLACK);
        UG_TextboxSetText(&window1, id, "");
    }

	if (fileCount < 1)
	{
		const char* text = "(empty)";

        uint16_t id = TXB_ID_0 + (ITEM_COUNT / 2);
        UG_TextboxSetText(&window1, id, (char*)text);

        UpdateDisplay();
	}
	else
	{
        char* displayStrings[ITEM_COUNT];
        for(int i = 0; i < ITEM_COUNT; ++i)
        {
            displayStrings[i] = NULL;
        }

	    for (int line = 0; line < ITEM_COUNT; ++line)
	    {
			if (page + line >= fileCount) break;

            uint16_t id = TXB_ID_0 + line;

	        if ((page) + line == currentItem)
	        {
	            UG_TextboxSetForeColor(&window1, id, C_BLACK);
                UG_TextboxSetBackColor(&window1, id, C_YELLOW);
	        }
	        else
	        {
	            UG_TextboxSetForeColor(&window1, id, C_BLACK);
                UG_TextboxSetBackColor(&window1, id, C_WHITE);
	        }

			char* fileName = files[page + line];
			if (!fileName) abort();

            size_t fileNameLength = strlen(fileName) - 4; // remove extension
            size_t displayLength = (fileNameLength <= MAX_DISPLAY_LENGTH) ? fileNameLength : MAX_DISPLAY_LENGTH;

            displayStrings[line] = (char*)heap_caps_malloc(displayLength + 1, MALLOC_CAP_SPIRAM);
            if (!displayStrings[line]) abort();

            strncpy(displayStrings[line], fileName, displayLength);
            displayStrings[line][displayLength] = 0; // NULL terminate

	        UG_TextboxSetText(&window1, id, displayStrings[line]);
	    }

        UpdateDisplay();

        for(int i = 0; i < ITEM_COUNT; ++i)
        {
            free(displayStrings[i]);
        }
	}


}

static const char* ChooseFile()
{
    const char* result = NULL;

    fb = (uint16_t*)heap_caps_malloc(320 * 240 * 2, MALLOC_CAP_SPIRAM);
    if (!fb) abort();
    //fb = (uint16_t*)ESP32_PSRAM;

    UG_Init(&gui, pset, 320, 240);

    UG_WindowCreate(&window1, objbuffwnd1, MAX_OBJECTS, window1callback);

    UG_WindowSetTitleText(&window1, "SELECT A RETRODROID");
    UG_WindowSetTitleTextFont(&window1, &FONT_10X16);
    UG_WindowSetTitleTextAlignment(&window1, ALIGN_CENTER);


    UG_S16 innerWidth = UG_WindowGetInnerWidth(&window1);
    UG_S16 innerHeight = UG_WindowGetInnerHeight(&window1);
    UG_S16 titleHeight = UG_WindowGetTitleHeight(&window1);
    UG_S16 textHeight = (innerHeight) / ITEM_COUNT;


    for (int i = 0; i < ITEM_COUNT; ++i)
    {
        uint16_t id = TXB_ID_0 + i;
        UG_S16 top = i * textHeight;
        UG_TextboxCreate(&window1, &textbox[i], id, 0, top, innerWidth, top + textHeight - 1);
        UG_TextboxSetFont(&window1, id, &FONT_8X12);
        UG_TextboxSetForeColor(&window1, id, C_BLACK);
        UG_TextboxSetAlignment(&window1, id, ALIGN_CENTER);
        //UG_TextboxSetText(&window1, id, "ABCDEFGHabcdefg");
    }

    UG_WindowShow(&window1);
    UpdateDisplay();


    const char* path = "/sd/roms/tap";
    char** files;

    int fileCount =  GetFiles(path, &files);


// Selection
    int currentItem = 0;
    DrawPage(files, fileCount, currentItem);

    odroid_gamepad_state previousState;
    odroid_input_gamepad_read(&previousState);

    while (true)
    {
		odroid_gamepad_state state;
		odroid_input_gamepad_read(&state);

        int page = currentItem / 10;
        page *= 10;

		if (fileCount > 0)
		{
	        if(!previousState.values[ODROID_INPUT_DOWN] && state.values[ODROID_INPUT_DOWN])
	        {
	            if (fileCount > 0)
				{
					if (currentItem + 1 < fileCount)
		            {
		                ++currentItem;
		                DrawPage(files, fileCount, currentItem);
		            }
					else
					{
						currentItem = 0;
		                DrawPage(files, fileCount, currentItem);
					}
				}
	        }
	        else if(!previousState.values[ODROID_INPUT_UP] && state.values[ODROID_INPUT_UP])
	        {
	            if (fileCount > 0)
				{
					if (currentItem > 0)
		            {
		                --currentItem;
		                DrawPage(files, fileCount, currentItem);
		            }
					else
					{
						currentItem = fileCount - 1;
						DrawPage(files, fileCount, currentItem);
					}
				}
	        }
	        else if(!previousState.values[ODROID_INPUT_RIGHT] && state.values[ODROID_INPUT_RIGHT])
	        {
	            if (fileCount > 0)
				{
					if (page + 10 < fileCount)
		            {
		                currentItem = page + 10;
		                DrawPage(files, fileCount, currentItem);
		            }
					else
					{
						currentItem = 0;
						DrawPage(files, fileCount, currentItem);
					}
				}
	        }
	        else if(!previousState.values[ODROID_INPUT_LEFT] && state.values[ODROID_INPUT_LEFT])
	        {
	            if (fileCount > 0)
				{
					if (page - 10 >= 0)
		            {
		                currentItem = page - 10;
		                DrawPage(files, fileCount, currentItem);
		            }
					else
					{
						currentItem = page;
						while (currentItem + 10 < fileCount)
						{
							currentItem += 10;
						}

		                DrawPage(files, fileCount, currentItem);
					}
				}
	        }
	        else if(!previousState.values[ODROID_INPUT_A] && state.values[ODROID_INPUT_A])
	        {
	            size_t fullPathLength = strlen(path) + 1 + strlen(files[currentItem]) + 1;

	            //char* fullPath = (char*)heap_caps_malloc(fullPathLength, MALLOC_CAP_SPIRAM);
                char* fullPath = (char*)malloc(fullPathLength);
	            if (!fullPath) abort();

	            strcpy(fullPath, path);
	            strcat(fullPath, "/");
	            strcat(fullPath, files[currentItem]);

	            result = fullPath;
                break;
	        }
		}

        previousState = state;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    FreeFiles(files, fileCount);

    //free(fb);
    return result;
}

static uint32_t tiaSamplesPerFrame;

void retrOdroid_init(const char* filename)
{
    printf("%s: HEAP:0x%x (%#08x)\n",
      __func__,
      esp_get_free_heap_size(),
      heap_caps_get_free_size(MALLOC_CAP_DMA));

    const char* result = NULL;
    static const size_t MAX_DISPLAY_LENGTH = 38;

    fb = (uint16_t*)heap_caps_malloc(320 * 240 * 2, MALLOC_CAP_SPIRAM);
    if (!fb) abort();
    //fb = (uint16_t*)ESP32_PSRAM;

    char * fileName = "Testing 123";

    printf("Actual File: %s\n", fileName);


    UG_Init(&gui, pset, 320, 240);

    UG_WindowCreate(&window1, objbuffwnd1, MAX_OBJECTS, window1callback);

    size_t fileNameLength = strlen(fileName) - 4; // remove extension
    size_t displayLength = (fileNameLength <= MAX_DISPLAY_LENGTH) ? fileNameLength : MAX_DISPLAY_LENGTH;

    char * displayStrings = (char*)heap_caps_malloc(displayLength + 1, MALLOC_CAP_SPIRAM);
    if (!displayStrings) abort();

    strncpy(displayStrings, fileName, displayLength);
    displayStrings[displayLength] = 0; // NULL terminate

    printf("File: %s\n", displayStrings);

    UG_WindowSetTitleText(&window1, displayStrings);
    UG_WindowSetTitleTextFont(&window1, &FONT_10X16);
    UG_WindowSetTitleTextAlignment(&window1, ALIGN_CENTER);

    UG_WindowShow(&window1);
    UpdateDisplay();

    odroid_gamepad_state previousState;
    odroid_input_gamepad_read(&previousState);

    int ll = 40;

    float frequency[] = {
      659.25511, 493.8833, 523.25113, 587.32954, 523.25113, 493.8833, 440.0, 440.0, 523.25113, 659.25511, 587.32954, 523.25113, 493.8833, 523.25113, 587.32954, 659.25511, 523.25113, 440.0, 440.0, 440.0, 493.8833, 523.25113, 587.32954, 698.45646, 880.0, 783.99087, 698.45646, 659.25511, 523.25113, 659.25511, 587.32954, 523.25113, 493.8833, 493.8833, 523.25113, 587.32954, 659.25511, 523.25113, 440.0, 440.0
    };

    float duration[] = {
        406.250, 203.125, 203.125, 406.250, 203.125, 203.125, 406.250, 203.125, 203.125, 406.250, 203.125, 203.125, 609.375, 203.125, 406.250, 406.250, 406.250, 406.250, 203.125, 203.125, 203.125, 203.125, 609.375, 203.125, 406.250, 203.125, 203.125, 609.375, 203.125, 406.250, 203.125, 203.125, 406.250, 203.125, 203.125, 406.250, 406.250, 406.250, 406.250, 406.250
    };

    while (true)
    {
		    odroid_gamepad_state state;
		    odroid_input_gamepad_read(&state);

        if (!previousState.values[ODROID_INPUT_MENU] && state.values[ODROID_INPUT_MENU])
        {
            esp_restart();
        }else if (!previousState.values[ODROID_INPUT_A] && state.values[ODROID_INPUT_A])
        {
            printf("PLAY WAVE 1\n");
            for (int i=0; i < ll; i++){
              //playWave(1000, frequency[i], (duration[i]/1000)); //5 KHz test
              playWave2(sine, WAV_SIZE, frequency[i], (duration[i]/1000));

            }
        }else if (!previousState.values[ODROID_INPUT_B] && state.values[ODROID_INPUT_B])
        {
            printf("PLAY WAVE 2\n");
            playWave2(sine, WAV_SIZE, 2000, 2);

        }else if (!previousState.values[ODROID_INPUT_VOLUME] && state.values[ODROID_INPUT_VOLUME])
        {
          odroid_audio_terminate();
          if (AudioSink == ODROID_AUDIO_SINK_DAC){
            AudioSink = ODROID_AUDIO_SINK_SPEAKER;
            odroid_audio_init(ODROID_AUDIO_SINK_SPEAKER, AUDIO_SAMPLE_RATE);
          }else if (AudioSink == ODROID_AUDIO_SINK_SPEAKER){
            AudioSink = ODROID_AUDIO_SINK_DAC;
            odroid_audio_init(ODROID_AUDIO_SINK_DAC, AUDIO_SAMPLE_RATE);
          }
          odroid_audio_volume_set(ODROID_VOLUME_LEVEL0);
        }

        previousState = state;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

}

void retrOdroid_step(odroid_gamepad_state* gamepad)
{

}

bool RenderFlag;
extern "C" void app_main()
{
    printf("retrOdroid-go started.\n");

    printf("HEAP:0x%x (%#08x)\n",
      esp_get_free_heap_size(),
      heap_caps_get_free_size(MALLOC_CAP_DMA));


    nvs_flash_init();

    odroid_system_init();
    odroid_input_gamepad_init();
    odroid_input_battery_level_init();

    ili9341_prepare();

    ili9341_init();
    ili9341_clear(0x0000);

    generateSine(AMPLITUDE, sine, WAV_SIZE);

    //vTaskDelay(500 / portTICK_RATE_MS);

    // Open SD card
    esp_err_t r = odroid_sdcard_open(SD_BASE_PATH);
    if (r != ESP_OK)
    {
        odroid_display_show_sderr(ODROID_SD_ERR_NOCARD);
        abort();
    }


    //const char* romfile = ChooseFile();
    const char* romfile = "TEST";
    //printf("%s: filename='%s'\n", __func__, romfile);


    ili9341_clear(0x0000);

    odroid_audio_init(ODROID_AUDIO_SINK_DAC, AUDIO_SAMPLE_RATE);
    odroid_audio_volume_set(ODROID_VOLUME_LEVEL0);

    retrOdroid_init(romfile);


    vidQueue = xQueueCreate(1, sizeof(uint16_t*));
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 1024 * 4, NULL, 5, NULL, 1);


    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    int frame = 0;
    int renderFrames = 0;
    uint16_t muteFrameCount = 0;
    uint16_t powerFrameCount = 0;

    odroid_gamepad_state last_gamepad;
    odroid_input_gamepad_read(&last_gamepad);

    static const bool renderTable[8] = {
        true, false,
        false, true,
        true, false,
        false, true };

    while(1)
    {
        startTime = xthal_get_ccount();


        odroid_gamepad_state gamepad;
        odroid_input_gamepad_read(&gamepad);

        if (last_gamepad.values[ODROID_INPUT_MENU] &&
            !gamepad.values[ODROID_INPUT_MENU])
        {
            esp_restart();
        }

        if (!last_gamepad.values[ODROID_INPUT_VOLUME] &&
            gamepad.values[ODROID_INPUT_VOLUME])
        {
            odroid_audio_volume_change();
            printf("%s: Volume=%d\n", __func__, odroid_audio_volume_get());
        }


        RenderFlag = renderTable[frame & 7];
        retrOdroid_step(&gamepad);
        //printf("stepped.\n");


        if (RenderFlag)
        {
            //TIA& tia = console->tia();
            //uint8_t* fb = tia.currentFrameBuffer();

            //xQueueSend(vidQueue, &fb, portMAX_DELAY);

            ++renderFrames;
        }

        last_gamepad = gamepad;


        // end of frame
        stopTime = xthal_get_ccount();


        odroid_battery_state battery;
        odroid_input_battery_level_read(&battery);


        int elapsedTime;
        if (stopTime > startTime)
          elapsedTime = (stopTime - startTime);
        else
          elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);

        totalElapsedTime += elapsedTime;
        ++frame;

        if (frame == 60)
        {
          float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f);
          float fps = frame / seconds;
          float renderFps = renderFrames / seconds;

          printf("HEAP:0x%x (%#08x), SIM:%f, REN:%f, BATTERY:%d [%d]\n",
            esp_get_free_heap_size(),
            heap_caps_get_free_size(MALLOC_CAP_DMA),
            fps,
            renderFps,
            battery.millivolts,
            battery.percentage);

          frame = 0;
          renderFrames = 0;
          totalElapsedTime = 0;
        }
    }
}

