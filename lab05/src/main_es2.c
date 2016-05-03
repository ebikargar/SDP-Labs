// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
//           Made for JamesM's tutorials <www.jamesmolloy.co.uk>

#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"

int main(struct multiboot *mboot_ptr)
{
    // Initialise all the ISRs and segmentation
    init_descriptor_tables();
    // Initialise the screen (by clearing it)
    monitor_clear();

    initialise_paging();
    monitor_write("Hello, paging world!\n");

    u32int num_pages = 160; //0 to 264
    // accessing pages with high number leads to some strange behavior
    // starting to print empty or wrong values
    // and finally breaking the flow of instructions
    // making the OS go to a strage state
    u32int i = 0;
    u32int *ptr = 0;
    while(i < num_pages) {
      *ptr = i;
      monitor_write("ptr ");
      monitor_write_hex(ptr);
      monitor_write(" (page ");
      monitor_write_dec(i);
      monitor_write(") contains ");
      monitor_write_dec(*ptr);
      monitor_write("\n");
      ptr = ptr + (u32int)0x400;
      i++;
  }

  // call swap frames 0, 1
  swap_pages(0,1);
  // and read values

  ptr = 0;
  //page 0 now contains 1
  monitor_write("page 0 now contains ");
  monitor_write_dec(*ptr);
  monitor_write("\npage 1 now contains ");
  ptr = (u32int)0x1000;
  monitor_write_dec(*ptr);
  monitor_write("\n");

  return 0;
}
