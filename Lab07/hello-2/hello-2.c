/*
 * hello-2.c  Demonstrating the module_init() and module_exit() macros.
 * This is preferred over using init_module() and cleanup_module().
 */
#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
static int __init hello_2_init(void)
{
        printk(KERN_INFO "Hello, world 2\n");
        return 0;
}
static void __exit hello_2_exit(void)
{
        printk(KERN_INFO "Goodbye, world 2\n");
}
module_init(hello_2_init);
module_exit(hello_2_exit);

/* The __init  is a hint to the kernel that the given function is
 * The module loader drops the initialization function
 * after the module is loaded, making its memory available

 * The __exit marks the code as being for module unload only
 * (by causing the compiler to place it in a special ELF section)
 */