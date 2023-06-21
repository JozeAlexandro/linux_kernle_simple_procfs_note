#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include <stdbool.h>


/**  
 @note Начиная с этой версии ядра введена структура struct proc_ops. 
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif 


/**  
 @brief Имя файла в /proc для взаимодействия пользователя с модулем ядра. 
 */
#define PROCFS_FILE_NAME "procfs_3_note"

/**  
 @brief Максимальный размер буфера. 
 */
enum { MAX_FILE_SIZE = 2048 };


/**  
 @brief "Точка входа" для взаимодействия пользователя с модулем. Файл в /proc. 
 */
static struct proc_dir_entry *ourProcFile;

/**  
 @brief Внутренний буфер модуля. 
 */
static char procfsBuffer[ MAX_FILE_SIZE ] = { 0 };

/**  
 @brief Текущий размер буфера. 
 */
static unsigned procfsBufferSize = 0;



/**  
 @brief Макросы для удобного лога
 */
#define LOG_HELPER(fmt, ...) \
{ \
    pr_info( "%s: " fmt "%s\n", PROCFS_FILE_NAME, __VA_ARGS__ ); \
}

#define LOG(...) LOG_HELPER(__VA_ARGS__, "")



/**  
 @brief Функция чтения данных пользователем.
 
 @param [in] pFile Представление открытого файла.
 @param [in] buffer Массив пользовательских данных.
 @param [in] bufLen Длина массива пользовательских данных.
 @param [out] offset Сдвиг для чтения.
 
 @return -EFAULT в случае ошибки, 0 в случае конца чтения, 
         >0 (размер считанных данных в байтах) в случае частичного чтения.
 */
static ssize_t procFileRead( struct file *pFile, char __user *buffer,
                             size_t bufLen, loff_t *offset )
{
    enum { END_OF_READING = 0 };
    
    // Обработка случая окончания чтения
    if( *offset >= procfsBufferSize || 0 == procfsBufferSize )
    {
        LOG("procFileRead: end of reading");
        return END_OF_READING;
    }
    
    if( bufLen > procfsBufferSize )
    {
        bufLen = procfsBufferSize;
    }
    
    // Передача данных между пространствами пользователя и ядра
    if( copy_to_user( buffer, procfsBuffer, bufLen ) )
    {
        return -EFAULT;
    }
        
    LOG( "procFileRead: read %lu bytes", bufLen );
    *offset += bufLen;

    return bufLen;    
}

/**  
 @brief Функция записи данных пользователем.
 
 @param [in] pFile Представление открытого файла.
 @param [out] buffer Массив для записи данных пользователю.
 @param [in] len Длина массива  данных.
 @param [out] offset Сдвиг для записи.
 
 @return -EFAULT в случае ошибки, >0 (размер записанных данных в байтах) 
         в случае частичной записи.
 */
static ssize_t procFileWrite( struct file *pFile, const char __user *buff, 
                              size_t len, loff_t *off) 
{
    if( len > MAX_FILE_SIZE  )
    {
        len = MAX_FILE_SIZE;
        LOG( "procFileWrite: file is truncated to %lu bytes", len );
    }
    else
    {
        LOG( "procFileWrite: writing %lu bytes...", len );
    }
    
    // Передача данных между пространствами пользователя и ядра
    if( copy_from_user( procfsBuffer, buff, len ) )
    {
        return -EFAULT;
    }
    
    *off += len;
    
    // Обновление размера внутреннего буфера
    procfsBufferSize = len;
    
    LOG( "procFileWrite: writing done!" );
    
    return len;
}

/**  
 @brief Функция открытия файла в /proc.

 @param [in] inode Информация о файле/директории.
 @param [in] file Представление открытого файла.
 */
static int procfsOpen( struct inode *inode, struct file *file )
{
    LOG( "procfsOpen" );
    
    // Увеличение числа активных обращений к модулю. 
    // Нужно для корректной выгрузки
    try_module_get( THIS_MODULE );
    return 0;
}

/**  
 @brief Функция закрытия файла в /proc.

 @param [in] inode Информация о файле/директории.
 @param [in] file Представление открытого файла.
 */
static int procfsClose( struct inode *inode, struct file *file )
{
    LOG( "procfsClose" );
    
    // Уменьшение числа активных обращений к модулю. 
    module_put( THIS_MODULE );
    return 0;
}


#ifdef HAVE_PROC_OPS
static const struct proc_ops procFileOps = 
{
    .proc_read = procFileRead,
    .proc_write = procFileWrite,
    .proc_open = procfsOpen,
    .proc_release = procfsClose
};
#else 
static const struct file_operations procFileOps = 
{
    .read = procFileRead,
    .write = procFileWrite,
    .open = procfsOpen,
    .release = procfsClose
};
#endif 

/**  
 @brief Функция загрузки модуля.
 */
static int __init procfs2Init( void )
{
    ourProcFile = proc_create( PROCFS_FILE_NAME, 0666, NULL, &procFileOps );
    if( NULL == ourProcFile )
    {
        proc_remove( ourProcFile );
        LOG( "Error: Could not initialize /proc/%s", PROCFS_FILE_NAME );
        return -ENOMEM;
    }
    
    // Установка размера файла /proc/*
    /// @todo Зачем?
    proc_set_size( ourProcFile, 8 );
    
    // Установка идентификаторов владения файла /proc/*
    proc_set_user( ourProcFile, GLOBAL_ROOT_UID, GLOBAL_ROOT_GID );
    
    LOG( "/proc/%s created", PROCFS_FILE_NAME );
    return 0;
}

/**  
 @brief Функция выгрузки модуля.
 */
static void __exit procfs2Exit( void )
{
    proc_remove( ourProcFile );
    LOG( "/proc/%s removed", PROCFS_FILE_NAME );
}


module_init( procfs2Init );
module_exit( procfs2Exit );

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR("Alexandr Petrov");
MODULE_DESCRIPTION("procfs example");


