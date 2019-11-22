#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/uio.h>
#include <linux/fs.h>

#define MAX_DEV 2

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

struct process_vm_dev {
    atomic_t atomic_fastpath;

    struct semaphore reader_semaphore;
    char __user *reader_buf;
    size_t reader_count;
    pid_t reader_pid;

    struct semaphore writer_semaphore;
    size_t writer_count;

    struct cdev cdev;
};

static int dev_major = 0;
static struct class *process_vm_pipe_class = NULL;
static struct process_vm_dev process_vm_pipe_data[MAX_DEV];

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

        atomic_set(&process_vm_pipe_data[i].atomic_fastpath, STATE_INITIAL);
        sema_init(&process_vm_pipe_data[i].reader_semaphore, 0);
        sema_init(&process_vm_pipe_data[i].writer_semaphore, 0);
    }

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
    struct process_vm_dev *dev;

    dev = container_of(inode->i_cdev, struct process_vm_dev, cdev);
    file->private_data = dev;
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
    struct process_vm_dev *dev;
    atomic_t *atomic_fastpath;

    if (count == 0) {
        return 0;
    }

    if (count > MAX_RW_COUNT) {
        count = MAX_RW_COUNT;
    }

    if (!access_ok(VERIFY_READ, buf, count)) {
        return -EFAULT;
    }

    dev = file->private_data;
    dev->reader_buf = buf;
    dev->reader_count = count;
    dev->reader_pid = current->pid;
    barrier();

    atomic_fastpath = &dev->atomic_fastpath;
    state = atomic_cmpxchg(atomic_fastpath, STATE_INITIAL, STATE_READER_WAIT);

    if (state == STATE_INITIAL) {
        for (i = 0; i < SPIN_ITERATIONS; i++) {
            state = atomic_read(atomic_fastpath);
            if (state == STATE_WRITER_START) {
                break;
            }
        }
    }

    state = atomic_cmpxchg(atomic_fastpath, STATE_READER_WAIT, STATE_INITIAL);

    if (state == STATE_READER_WAIT) {
        up(&dev->writer_semaphore);
        if (down_interruptible(&dev->reader_semaphore)) {
            return -ERESTARTSYS;
        }
    } else {
        while (state != STATE_WRITER_FINISH) {
            state = atomic_read(atomic_fastpath);
        }
    }

    barrier();

    count = dev->writer_count;
    atomic_cmpxchg(atomic_fastpath, STATE_WRITER_FINISH, STATE_INITIAL);

    if (count == 0) {
        return -EIO;
    }
    return count;
}

static ssize_t process_vm_pipe_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    int i, state;
    struct iovec local_iov, remote_iov;
    struct process_vm_dev *dev;
    atomic_t *atomic_fastpath;

    if (count == 0) {
        return 0;
    }

    if (count > MAX_RW_COUNT) {
        count = MAX_RW_COUNT;
    }

    if (!access_ok(VERIFY_WRITE, buf, count)) {
        return -EFAULT;
    }

    dev = file->private_data;
    atomic_fastpath = &dev->atomic_fastpath;

    for (i = 0; i < SPIN_ITERATIONS; i++) {
        state = atomic_read(atomic_fastpath);
        if (state == STATE_READER_WAIT) {
            break;
        }
    }
    state = atomic_cmpxchg(atomic_fastpath, STATE_READER_WAIT, STATE_WRITER_START);
    if (state != STATE_READER_WAIT) {
        if (down_interruptible(&dev->writer_semaphore)) {
            return -ERESTARTSYS;
        }
    }

    if (count > dev->reader_count) {
        count = dev->reader_count;
    }

    barrier();

    local_iov.iov_base = buf;
    local_iov.iov_len = count;
    remote_iov.iov_base = dev->reader_buf;
    remote_iov.iov_len = count;

    count = ksys_process_vm_writev(dev->reader_pid, &local_iov, 1, &remote_iov, 1, 0);

    dev->writer_count = count;

    barrier();

    if (atomic_cmpxchg(atomic_fastpath, STATE_WRITER_START, STATE_WRITER_FINISH) != STATE_WRITER_START) {
        up(&dev->reader_semaphore);
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
