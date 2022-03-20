#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0xb3753869, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xe6278ff5, "__register_chrdev" },
	{ 0x4302d0eb, "free_pages" },
	{ 0x6a5cb5ee, "__get_free_pages" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x2d3385d3, "system_wq" },
	{ 0x1937241e, "try_module_get" },
	{ 0x3ba859c1, "module_put" },
	{ 0x7a2af7b4, "cpu_number" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x9166fada, "strncpy" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xf5cb25c8, "kmem_cache_alloc_trace" },
	{ 0x35216b26, "kmalloc_caches" },
	{ 0x37a0cba, "kfree" },
	{ 0xb44ad4b3, "_copy_to_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x1000e51, "schedule" },
	{ 0xa0c6befa, "hrtimer_cancel" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xfbdfc558, "hrtimer_start_range_ns" },
	{ 0x1ee7d3cd, "hrtimer_init" },
	{ 0x1b44c663, "current_task" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x64c17a3f, "wake_up_process" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "0C48CCEA9A341996B2B63DE");
