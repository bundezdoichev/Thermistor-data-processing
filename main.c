#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#if defined(CY_DEVICE_PSOC6A512K)  /* if the target is CY8CPROTO-062S3-4343W */
/* Channel 0 input pin */
#define VPLUS_CHANNEL_0  (P10_3)
#else 
#define VPLUS_CHANNEL_0  (P10_0)
#endif

/* Conversion factor */
#define MICRO_TO_MILLI_CONV_RATIO        (1000u)

/* Acquistion time in nanosecond */
#define ACQUISITION_TIME_NS              (1000u)

/* ADC Scan delay in millisecond */
#define ADC_SCAN_DELAY_MS                (200u)

/*******************************************************************************
*       Enumerated Types
*******************************************************************************/
/* ADC Channel constants*/
enum ADC_CHANNELS
{
  CHANNEL_0 = 0,
  CHANNEL_1,
  NUM_CHANNELS
} adc_channel;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
/* Single channel initialization function*/
void adc_single_channel_init(void);

/* Function to read input voltage from channel 0 */
void adc_single_channel_process(void);

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Variable for storing character read from terminal */
uint8_t uart_read_value;

/* ADC Object */
cyhal_adc_t adc_obj;

/* ADC Channel 0 Object */
cyhal_adc_channel_t adc_chan_0_obj;

/* Default ADC configuration */
const cyhal_adc_config_t adc_config = {
        .continuous_scanning=false, // Continuous Scanning is disabled
        .average_count=1,           // Average count disabled
        .vref=CYHAL_ADC_REF_VDDA,   // VREF for Single ended channel set to VDDA
        .vneg=CYHAL_ADC_VNEG_VSSA,  // VNEG for Single ended channel set to VSSA
        .resolution = 12u,          // 12-bit resolution
        .ext_vref = NC,             // No connection
        .bypass_pin = NC };       // No connection


int main(void)
{
    /* Variable to capture return value of functions */
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    
    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                 CY_RETARGET_IO_BAUDRATE);

    /* retarget-io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Print message */
    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("-----------------------------------------------------------\r\n");
    printf("PSoC 6 MCU: ADC using HAL\r\n");
    printf("-----------------------------------------------------------\r\n\n");

    /* Initialize Channel 0 */
    adc_single_channel_init();

    /* Update ADC configuration */
    result = cyhal_adc_configure(&adc_obj, &adc_config);
    if (result != CY_RSLT_SUCCESS)
    {
        printf("ADC configuration update failed. Error: %ld\n", (long unsigned int)result);
        CY_ASSERT(0);
    }

    /* delay value for reading data */
    uint32_t adc_scan_delay_ms = ADC_SCAN_DELAY_MS;
    /* whether or not to read data */
    uint8_t read_flag = 1;

    for (;;)
    {
        if (cyhal_uart_getc(&cy_retarget_io_uart_obj, &uart_read_value, 1) 
             == CY_RSLT_SUCCESS)
        {
            if (uart_read_value == '1') {
                read_flag = read_flag ? 0 : 1;
            } else if (uart_read_value == '2') {
                /* read delay value from user */
                if (cyhal_uart_getc(&cy_retarget_io_uart_obj, &adc_scan_delay_ms, 0)
                     == CY_RSLT_SUCCESS) {
                    /* get int from ASCII code */
                    adc_scan_delay_ms -= 48;
                    /* get millisecond from seconds */
                    adc_scan_delay_ms *= 1000;
                }
            }
        }

        if (read_flag) {
            /* Sample input voltage at channel 0 */
            adc_single_channel_process();

            /* delay between scans */
            cyhal_system_delay_ms(adc_scan_delay_ms);
        }
    }
}


void adc_single_channel_process(void)
{
    /* Variable to store ADC conversion result from channel 0 */
    int32_t adc_result_0 = 0;

    /* Read input voltage, convert it to millivolts and print input voltage */
    adc_result_0 = cyhal_adc_read_uv(&adc_chan_0_obj) / MICRO_TO_MILLI_CONV_RATIO;
    printf("Channel 0 input: %4ldmV\r\n", (long int)adc_result_0);
}


void adc_single_channel_init(void)
{
    /* Variable to capture return value of functions */
    cy_rslt_t result;

    /* Initialize ADC. The ADC block which can connect to the channel 0 input pin is selected */
    result = cyhal_adc_init(&adc_obj, VPLUS_CHANNEL_0, NULL);
    if(result != CY_RSLT_SUCCESS)
    {
        printf("ADC initialization failed. Error: %ld\n", (long unsigned int)result);
        CY_ASSERT(0);
    }

    /* ADC channel configuration */
    const cyhal_adc_channel_config_t channel_config = {
            .enable_averaging = false,  // Disable averaging for channel
            .min_acquisition_ns = ACQUISITION_TIME_NS, // Minimum acquisition time set to 1us
            .enabled = true };          // Sample this channel when ADC performs a scan

    /* Initialize a channel 0 and configure it to scan the channel 0 input pin in single ended mode. */
    result  = cyhal_adc_channel_init_diff(&adc_chan_0_obj, &adc_obj, VPLUS_CHANNEL_0,
                                          CYHAL_ADC_VNEG, &channel_config);
    if(result != CY_RSLT_SUCCESS)
    {
        printf("ADC single ended channel initialization failed. Error: %ld\n", (long unsigned int)result);
        CY_ASSERT(0);
    }

    printf("ADC is configured in single channel configuration\r\n\n");
    printf("Provide input voltage at the channel 0 input pin. \r\n\n");
}
