<script>
  var $table = $('#table')
  function mounted() {
    $table.bootstrapTable({
      url: 'json/treegrid.json',
      striped: true,
      sidePagination: 'server',
      idField: 'id',
      showColumns: true,
      columns: [
        {
          field: 'ck',
          checkbox: true
        },
        {
          field: 'name',
          title: '名称'
        },
        {
          field: 'status',
          title: '状态',
          sortable: true,
          align: 'center',
          formatter: 'statusFormatter'
        },
        {
          field: 'permissionValue',
          title: '权限值'
        }
      ],
      treeShowField: 'name',
      parentIdField: 'pid',
      onPostBody: function() {
        var columns = $table.bootstrapTable('getOptions').columns
        if (columns && columns[0][1].visible) {
          $table.treegrid({
            treeColumn: 1,
            onChange: function() {
              $table.bootstrapTable('resetWidth')
            }
          })
        }
      }
    })
  }
  function typeFormatter(value, row, index) {
    if (value === 'menu') {
      return '菜单'
    }
    if (value === 'button') {
      return '按钮'
    }
    if (value === 'api') {
      return '接口'
    }
    return '-'
  }
  function statusFormatter(value, row, index) {
    if (value === 1) {
      return '<span class="label label-success">正常</span>'
    }
    return '<span class="label label-default">锁定</span>'
  }
</script>
