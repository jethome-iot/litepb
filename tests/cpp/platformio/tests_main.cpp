extern int runTests();


#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
extern "C" void app_main() {
    // Wait 2 seconds for serial connection to stabilize
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    runTests();
}
#else
int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;
    return runTests();
}
#endif