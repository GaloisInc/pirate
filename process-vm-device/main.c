#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/uio.h>
#include <linux/fs.h>

#define MAX_DEV 1

#define SPIN_ITERATIONS (10000)

#define STATE_INITIAL 0
#define STATE_READER_WAIT 1
#define STATE_WRITER_START 2
#define STATE_WRITER_FINISH 3

extern ssize_t ksys_process_vm_readv(pid_t pid, const struct iovec *lvec,
                              unsigned long liovcnt, const struct iovec *rvec,
                              unsigned long riovcnt, unsigned long flags);

extern ssize_t ksys_process_vm_writev(pid_t pid, const struct iovec *lvec,
                               unsigned long liovcnt, const struct iovec *rvec,
                               unsigned long riovcnt, unsigned long flags);

static int process_vm_pipe_open(struct inode *inode, struct file *file);
static int process_vm_pipe_release(struct inode *inode, struct file *file);
static long process_vm_pipe_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t process_vm_pipe_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t process_vm_pipe_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

static const struct file_operations process_vm_pipe_fops = {
    .owner      = THIS_MODULE,
    .open       = process_vm_pipe_open,
    .release    = process_vm_pipe_release,
    .unlocked_ioctl = process_vm_pipe_ioctl,
    .read       = process_vm_pipe_read,
    .write       = process_vm_pipe_write
};

struct mychar_device_data {
    struct cdev cdev;
};

static int dev_major = 0;
static struct class *process_vm_pipe_class = NULL;
static struct mychar_device_data process_vm_pipe_data[MAX_DEV];

static atomic_t atomic_fastpath = ATOMIC_INIT(STATE_INITIAL);

static struct semaphore reader_semaphore;
static char __user *reader_buf;
static size_t reader_count;
static pid_t reader_pid;

static struct semaphore writer_semaphore;
static size_t writer_count;

static int process_vm_pipe_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init process_vm_pipe_init(void)
{
    int err, i;
    dev_t dev;

    err = alloc_chrdev_region(&dev, 0, MAX_DEV, "processvm-pipe");

    dev_major = MAJOR(dev);

    process_vm_pipe_class = class_create(THIS_MODULE, "processvm-pipe");
    process_vm_pipe_class->dev_uevent = process_vm_pipe_uevent;

    for (i = 0; i < MAX_DEV; i++) {
        cdev_init(&process_vm_pipe_data[i].cdev, &process_vm_pipe_fops);
        process_vm_pipe_data[i].cdev.owner = THIS_MODULE;

        cdev_add(&process_vm_pipe_data[i].cdev, MKDEV(dev_major, i), 1);

        device_create(process_vm_pipe_class, NULL, MKDEV(dev_major, i), NULL, "processvm-pipe-%d", i);
    }

    sema_init(&reader_semaphore, 0);
    sema_init(&writer_semaphore, 0);

    return 0;
}

static void __exit process_vm_pipe_exit(void)
{
    int i;

    for (i = 0; i < MAX_DEV; i++) {
        device_destroy(process_vm_pipe_class, MKDEV(dev_major, i));
    }

    class_unregister(process_vm_pipe_class);
    class_destroy(process_vm_pipe_class);

    unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
}

static int process_vm_pipe_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int process_vm_pipe_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long process_vm_pipe_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static ssize_t process_vm_pipe_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int i, state;

    if (count == 0) {
        return 0;
    }

    if (count > MAX_RW_COUNT) {
        count = MAX_RW_COUNT;
    }

    if (!access_ok(VERIFY_READ, buf, count)) {
        return -EFAULT;
    }

    reader_buf = buf;
    reader_count = count;
    reader_pid = current->pid;

    barrier();

    state = atomic_cmpxchg(&atomic_fastpath, STATE_INITIAL, STATE_READER_WAIT);

    if (state == STATE_INITIAL) {
        for (i = 0; i < SPIN_ITERATIONS; i++) {
            state = atomic_read(&atomic_fastpath);
            if (state == STATE_WRITER_START) {
                break;
            }
        }
    }

    state = atomic_cmpxchg(&atomic_fastpath, STATE_READER_WAIT, STATE_INITIAL);

    if (state == STATE_READER_WAIT) {
        up(&writer_semaphore);
        if (down_interruptible(&reader_semaphore)) {
            return -ERESTARTSYS;
        }
    } else {
        while (state != STATE_WRITER_FINISH) {
            state = atomic_read(&atomic_fastpath);
        }
    }

    barrier();

    count = writer_count;
    atomic_cmpxchg(&atomic_fastpath, STATE_WRITER_FINISH, STATE_INITIAL);

    if (count == 0) {
        return -EIO;
    }
    return count;
}

static ssize_t process_vm_pipe_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    int i, state;
    struct iovec local_iov, remote_iov;

    if (count == 0) {
        return 0;
    }

    if (count > MAX_RW_COUNT) {
        count = MAX_RW_COUNT;
    }

    if (!access_ok(VERIFY_WRITE, buf, count)) {
        return -EFAULT;
    }

    for (i = 0; i < SPIN_ITERATIONS; i++) {
        state = atomic_read(&atomic_fastpath);
        if (state == STATE_READER_WAIT) {
            break;
        }
    }
    state = atomic_cmpxchg(&atomic_fastpath, STATE_READER_WAIT, STATE_WRITER_START);
    if (state != STATE_READER_WAIT) {
        if (down_interruptible(&writer_semaphore)) {
            return -ERESTARTSYS;
        }
    }

    if (count > reader_count) {
        count = reader_count;
    }

    barrier();

    local_iov.iov_base = buf;
    local_iov.iov_len = count;
    remote_iov.iov_base = reader_buf;
    remote_iov.iov_len = count;

    count = ksys_process_vm_writev(reader_pid, &local_iov, 1, &remote_iov, 1, 0);

    writer_count = count;

    barrier();

    if (atomic_cmpxchg(&atomic_fastpath, STATE_WRITER_START, STATE_WRITER_FINISH) != STATE_WRITER_START) {
        up(&reader_semaphore);
    }

    if (count == 0) {
        return -EIO;
    }
    return count;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Spiegel <michael.spiegel@twosixlabs.com>");

module_init(process_vm_pipe_init);
module_exit(process_vm_pipe_exit);
