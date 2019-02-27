#include <gio/gio.h>
#include <gtk/gtk.h>

/* 底层数据的各列索引 */
enum
{
    CHECKED_COL,    /* 第0列：待办项是否已完成，布尔值 */
    TEXT_COL,       /* 第1列：待办项内容，字符串 */
    N_COLUMNS       /* 总列数：2 */
};

/* 复选框鼠标点击事件响应函数 */
static void
on_toggle(GtkCellRendererToggle *toggle,
          gchar *path,
          gpointer user_data)
{
    /* 1. 从 path 中获取底层数据的行索引 */
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(user_data), &iter, path);

    /* 2. 根据行索引获取该行数据第0列的值（布尔值） */
    gboolean checked;
    gtk_tree_model_get(GTK_TREE_MODEL(user_data),            // 底层数据
                                      &iter,                 // 行索引
                                      CHECKED_COL, &checked, // 列索引，存放值的变量
                                      -1);                   // -1 结束参数列表

    /* 3. 将该行第0列的布尔值反转 */
    gtk_tree_store_set(GTK_TREE_STORE(user_data),            // 底层数据
                       &iter,                                // 行索引
                       CHECKED_COL, !checked,                // 列索引，新值
                       -1);                                  // -1 结束参数列表
}


/* 设置窗口属性 */
static void
set_window_properties (GtkWindow *window)
{
    gtk_window_set_title (window, "GTODO");
    gtk_window_set_default_size (window, 400, 400);
}


/* 添加一行数据 */
static void
append_todo_item (GtkTreeStore *treestore, gboolean checked, const gchar *text)
{
    /* 获取新一行数据的行索引 */
    GtkTreeIter iter;
    gtk_tree_store_append(treestore, &iter, NULL);

    /* 在该索引位置插入一行数据 */
    gtk_tree_store_set(treestore, &iter,
                       CHECKED_COL, checked,    /* 第0列 */
                       TEXT_COL, text,          /* 第1列 */
                       -1);
}


/* 从文件加载待办列表
 * 文件名为：list.txt
 * 文件格式为：
 *     - 每行为一个待办事项
 *     - 每行格式为：是否已完成 逗号 待办内容
 *     - 是否已完成由数字 0 或 1 表示：0未完成，1已完成
 *     - 逗号前后无空格
 *     - 待办内容为任意字符串（可以包含逗号）
 * 例子：
 * 
 * 0,Buy an apple
 * 1,Buy a pencil
 * 
 */
static void
load_list_from_file (GtkTreeStore *treestore)
{
    GFile *file;

    /* 查询文件是否存在 */
    file = g_file_new_for_path ("list.txt");
    if (!g_file_query_exists (file, NULL))
    {
        return;
    }

    /* 打开文件 */
    GFileInputStream *fin = g_file_read (file, NULL, NULL);
    GDataInputStream *din = g_data_input_stream_new (G_INPUT_STREAM (fin));

    gchar *line;
    gsize len;

    /* 按行读取文件 */
    line = g_data_input_stream_read_line_utf8 (din, &len, NULL, NULL);
    while (len > 0)
    {
        gchar **tokens;
        tokens = g_strsplit (line, ",", 2);
        gboolean checked = atoi(tokens[0]);
        gchar *text = tokens[1];
        append_todo_item (treestore, checked, text);

        line = g_data_input_stream_read_line_utf8 (din, &len, NULL, NULL);
    }

    g_object_unref (file);
    g_object_unref (fin);
    g_object_unref (din);
}


/* 写入一项待办事项到文件 */
static void
write_one_item (GDataOutputStream *dout, gboolean checked, gchar *text)
{
    if (checked)
    {
        g_data_output_stream_put_string (dout, "1,", NULL, NULL);
    }
    else
    {
        g_data_output_stream_put_string (dout, "0,", NULL, NULL);
    }
    g_data_output_stream_put_string (dout, text, NULL, NULL);
    g_data_output_stream_put_string (dout, "\n", NULL, NULL);
}


/* 将待办列表保存到文件 */
static void
save_list_to_file (GtkTreeModel *treemodel)
{
    GFile *file;
    GFileOutputStream *fout;
    GDataOutputStream *dout;

    /* 删除已存在的文件 */
    file = g_file_new_for_path ("list.txt");
    if (g_file_query_exists (file, NULL))
    {
        g_file_delete (file, NULL, NULL);
    }

    /* 创建新文件 */
    fout = g_file_create (file, G_FILE_CREATE_REPLACE_DESTINATION, NULL, NULL);
    dout = g_data_output_stream_new (G_OUTPUT_STREAM (fout));

    /* 读取底层数据 */
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first (treemodel, &iter);
    while (valid)
    {
        gboolean checked;
        gchararray text;
        gtk_tree_model_get (treemodel, &iter,
                            CHECKED_COL, &checked,
                            TEXT_COL, &text,
                            -1);
        
        /* 将一个待办事项写入文件一行 */
        write_one_item (dout, checked, text);

        /* 取下一个待办事项 */
        valid = gtk_tree_model_iter_next (treemodel, &iter);
    }

    g_object_unref (file);
    g_object_unref (fout);
    g_object_unref (dout);
}


/* 创建底层数据 */
static GtkTreeStore *
setup_tree_store ()
{
    GtkTreeStore *treestore;
    treestore = gtk_tree_store_new(N_COLUMNS,       /* 总列数：2 */
                                   G_TYPE_BOOLEAN,  /* 第0列：待办项是否已完成，布尔值 */
                                   G_TYPE_STRING    /* 第1列：待办项内容，字符串 */
                                   );

    /* 从文件加载待办列表 */
    load_list_from_file (treestore);

    return treestore;
}


/* 编辑待办事项后更新底层数据 */
void on_edited(GtkCellRendererText *renderer,
               gchar *path,
               gchar *new_text,
               gpointer user_data)
{
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (user_data),
                                         &iter,
                                         path);
    gtk_tree_store_set (GTK_TREE_STORE (user_data), &iter,
                       TEXT_COL, new_text,
                       -1);
}


/* 创建多列视图 */
static GtkWidget *
setup_tree_view (GtkTreeStore *treestore)
{
    GtkWidget *treeview;
    treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(treestore));

    /* 使视图中的待办事项可拖动 */
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (treeview), TRUE);

    /* 创建视图第0列的渲染器：复选框 */
    GtkCellRenderer *checked_renderer;
    checked_renderer = gtk_cell_renderer_toggle_new ();

    /* 为复选框的鼠标点击事件绑定响应函数 */
    g_signal_connect(checked_renderer,       // 信号发出者
                     "toggled",              // 信号名
                     G_CALLBACK (on_toggle), // 响应函数
                     treestore);                 // 传递给响应函数的自定义数据

    /* 创建视图第0列，启用渲染器，并将渲染器属性与底层数据绑定 */
    GtkTreeViewColumn *column0;
    column0 = gtk_tree_view_column_new_with_attributes (
        "Done",                 // 列名
        checked_renderer,       // 复选框渲染器
        "active", CHECKED_COL,  // 复选框的 active 属性（是否打钩）与底层数据绑定
        NULL);
    
    /* 向多列视图添加该列 */
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column0);

    /* 创建视图第1列的渲染器：文本框 */
    GtkCellRenderer *text_renderer;
    text_renderer = gtk_cell_renderer_text_new ();

    /* 文本框可双击编辑，编辑后更新底层数据 */
    g_object_set (text_renderer, "editable", TRUE, NULL);
    g_signal_connect (text_renderer, "edited",
                      G_CALLBACK (on_edited),
                      treestore);

    /* 创建视图第1列，启用渲染器，并将渲染器属性与底层数据绑定 */
    GtkTreeViewColumn *column1;
    column1 = gtk_tree_view_column_new_with_attributes (
        "To-do List",                   // 列名
        text_renderer,                  // 文本框渲染器
        "text", TEXT_COL,               // 文本框的 text 属性（文本内容）与底层数据绑定
        "strikethrough", CHECKED_COL,   // 文本框删除线属性
        NULL);
    
    /* 向多列视图添加该列 */
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column1);

    gtk_widget_set_hexpand (treeview, TRUE);
    gtk_widget_set_vexpand (treeview, TRUE);

    return treeview;
}


/* 点击输入框加号图标响应函数 */
void
on_icon_press (GtkEntry            *entry,
               GtkEntryIconPosition icon_pos,
               GdkEvent            *event,
               gpointer             user_data)
{
    /* 1. 获取输入框文本 */
    const gchar *text = gtk_entry_get_text (entry);
    if (g_utf8_strlen(text, 100) == 0)
    {
        return;
    }

    /* 2. 添加待办事项 */
    append_todo_item (GTK_TREE_STORE (user_data), FALSE, text);

    /* 3. 清空输入框 */
    gtk_entry_set_text (entry, "");
}


/* 窗口退出响应函数 */
gboolean
on_delete (GtkWidget *widget,
           GdkEvent  *event,
           gpointer   user_data)
{
    save_list_to_file (GTK_TREE_MODEL (user_data));
    gtk_widget_destroy (widget);
    return FALSE;
}


/* GApplication 的 activate 信号处理函数*/
static void
on_activate (GtkApplication* app,
             gpointer        user_data)
{
    GtkWidget *window;
    GtkTreeStore *treestore;
    GtkWidget *treeview;
    GtkWidget *grid;
    GtkWidget *entry;

    /* 创建窗口 */
    window = gtk_application_window_new (app);

    /* 设置窗口属性 */
    set_window_properties (GTK_WINDOW (window));

    /* 创建底层数据 */
    treestore = setup_tree_store ();
    
    /* 创建多列视图 */
    treeview = setup_tree_view (treestore);

    /* 创建网格布局 */
    grid = gtk_grid_new ();
    gtk_grid_set_row_spacing (GTK_GRID (grid), 10);
    gtk_grid_set_column_spacing (GTK_GRID (grid), 10);

    /* 向网格布局添加多列视图 */
    gtk_grid_attach (GTK_GRID (grid), treeview, 0, 0, 1, 1);  // x, y, width, height

    /* 创建输入框 */
    entry = gtk_entry_new ();

    /* 输入框显示加号，并绑定响应函数 */
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, "list-add-symbolic");
    g_signal_connect (entry, "icon-press", G_CALLBACK (on_icon_press), treestore);

    /* 向网格布局添加输入框 */
    gtk_grid_attach (GTK_GRID (grid), entry, 0, 1, 1, 1);

    /* 向窗口添加 Grid */
    gtk_container_add (GTK_CONTAINER (window), grid);

    /* 窗口退出时的响应函数 */
    g_signal_connect (window, "delete-event",
                      G_CALLBACK (on_delete),
                      treestore);
    
    /* 显示窗口 */
    gtk_widget_show_all (window);
}

int
main (int    argc,
      char **argv)
{
    GtkApplication *app;
    int status;

    /* 创建 GtkApplication 实例 */
    app = gtk_application_new ("com.github.jiaowenjun.gtodo", G_APPLICATION_FLAGS_NONE);

    /* 绑定 GtkApplication 的 activate 信号处理函数 */
    g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);

    /* 运行 GtkApplication */
    status = g_application_run (G_APPLICATION (app), argc, argv);

    /* 释放指针资源 */
    g_object_unref (app);

    return status;
}