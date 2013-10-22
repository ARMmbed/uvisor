#include <iot-os.h>
#include <usart.h>

void main(void)
{
    int i = 0;

    USART_init(12345);

    while(1)
    {
        printf("Hello World Nr. %i\n",i++);
    }
}
