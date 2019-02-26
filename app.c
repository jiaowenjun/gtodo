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

/* GApplication 的 activate 信号处理函数*/
static void
on_activate (GtkApplication* app,
             gpointer        user_data)
{
    /* 创建窗口 */
    GtkWidget *window;
    window = gtk_application_window_new (app);

    /* 设置窗口属性 */
    gtk_window_set_title (GTK_WINDOW (window), "Window");
    gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);

    /* 创建底层数据 */
    GtkTreeStore *store;
    store = gtk_tree_store_new(N_COLUMNS,       /* 总列数：2 */
                               G_TYPE_BOOLEAN,  /* 第0列：待办项是否已完成，布尔值 */
                               G_TYPE_STRING    /* 第1列：待办项内容，字符串 */
                               );

    /* 数据行索引（行下标） */
    GtkTreeIter iter;

    /* 获取新一行数据的行索引 */
    gtk_tree_store_append(store, &iter, NULL);

    /* 在该索引位置插入一行数据 */
    gtk_tree_store_set(store, &iter,
                       CHECKED_COL, FALSE,          /* 第0列 */
                       TEXT_COL, "Buy an apple",    /* 第1列 */
                       -1);

    /* 再添加一行数据 */
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter,
                       CHECKED_COL, FALSE,          /* 第0列 */
                       TEXT_COL, "Buy a pencil",    /* 第1列 */
                       -1);
    
    /* 创建多列视图 */
    GtkWidget *treeview;
    treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(store));

    /* 创建视图第0列的渲染器：复选框 */
    GtkCellRenderer *checked_renderer;
    checked_renderer = gtk_cell_renderer_toggle_new ();

    /* 为复选框的鼠标点击事件绑定响应函数 */
    g_signal_connect(checked_renderer,       // 信号发出者
                     "toggled",              // 信号名
                     G_CALLBACK (on_toggle), // 响应函数
                     store);                 // 传递给响应函数的自定义数据

    /* 创建视图第0列，启用渲染器，并将渲染器属性与底层数据绑定 */
    GtkTreeViewColumn *column0;
    column0 = gtk_tree_view_column_new_with_attributes (
        "已完成",                /* 列名 */
        checked_renderer,       /* 复选框渲染器 */
        "active", CHECKED_COL,  /* 复选框的 active 属性（是否打钩）与底层数据绑定 */
        NULL);
    
    /* 向多列视图添加该列 */
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column0);

    /* 创建视图第1列的渲染器：文本框 */
    GtkCellRenderer *text_renderer;
    text_renderer = gtk_cell_renderer_text_new ();

    /* 创建视图第1列，启用渲染器，并将渲染器属性与底层数据绑定 */
    GtkTreeViewColumn *column1;
    column1 = gtk_tree_view_column_new_with_attributes (
        "待办事项",              /* 列名 */
        text_renderer,          /* 文本框渲染器 */
        "text", TEXT_COL,       /* 文本框的 text 属性（文本内容）与底层数据绑定 */
        NULL);
    
    /* 向多列视图添加该列 */
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column1);

    /* 向窗口添加多列视图 */
    gtk_container_add (GTK_CONTAINER (window), treeview);
    
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
    app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);

    /* 绑定 GtkApplication 的 activate 信号处理函数 */
    g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);

    /* 运行 GtkApplication */
    status = g_application_run (G_APPLICATION (app), argc, argv);

    /* 释放指针资源 */
    g_object_unref (app);

    return status;
}