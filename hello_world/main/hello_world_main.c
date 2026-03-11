/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include <string.h>
#include <stdio.h>
#include <hwcrypto/aes.h>

/*
  For  Encryption time: 1802.40us  (9.09 MB/s) at 16kB blocks.
*/

static inline int32_t _getCycleCount(void) {
  int32_t ccount;
  __asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
  return ccount;
}

char plaintext[16384];
char encrypted[16384];


int encodetest()
{
	uint8_t key[32];
	uint8_t iv[16];

  //If you have cryptographically random data in the start of your payload, you do not need
	//an IV.  If you start a plaintext payload, you will need an IV.
	memset( iv, 0, sizeof( iv ) );

	//Right now, I am using a key of all zeroes.  This should change.  You should fill the key
  //out with actual data.
	memset( key, 0, sizeof( key ) );

	memset( plaintext, 0, sizeof( plaintext ) );
	strcpy( plaintext, "Hello, world, how are you doing today?" );

	//Just FYI - you must be encrypting/decrypting data that is in BLOCKSIZE chunks!!!

	esp_aes_context ctx;
	esp_aes_init( &ctx );
	esp_aes_setkey( &ctx, key, 256 );
	int32_t start = _getCycleCount();
	esp_aes_crypt_cbc( &ctx, ESP_AES_ENCRYPT, sizeof(plaintext), iv, (uint8_t*)plaintext, (uint8_t*)encrypted );
	int32_t end = _getCycleCount();
	float enctime = (end-start)/240.0;
	printf( "Encryption time: %.2fus  (%f MB/s)\n", enctime, (sizeof(plaintext)*1.0)/enctime );

	//See encrypted payload, and wipe out plaintext.
	memset( plaintext, 0, sizeof( plaintext ) );
	int i;
	for( i = 0; i < 128; i++ )
	{
		printf( "%02x[%c]%c", encrypted[i], (encrypted[i]>31)?encrypted[i]:' ', ((i&0xf)!=0xf)?' ':'\n' );
	}
	printf( "\n" );
	//Must reset IV.
	//XXX TODO: Research further: I found out if you don't reset the IV, the first block will fail
	//but subsequent blocks will pass.  Is there some strange cryptoalgebra going on that permits this?
	printf( "IV: %02x %02x\n", iv[0], iv[1] );
	memset( iv, 0, sizeof( iv ) );


	//Use the ESP32 to decrypt the CBC block.
	esp_aes_crypt_cbc( &ctx, ESP_AES_DECRYPT, sizeof(encrypted), iv, (uint8_t*)encrypted, (uint8_t*)plaintext );


	//Verify output
	for( i = 0; i < 128; i++ )
	{
		printf( "%02x[%c]%c", plaintext[i],  (plaintext[i]>31)?plaintext[i]:' ', ((i&0xf)!=0xf)?' ':'\n' );
	}
	printf( "\n" );


	esp_aes_free( &ctx );
}




void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
