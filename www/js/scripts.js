var mainInterval;

function cleanCurrent(tableName) {
    try {
        var table = document.getElementById(tableName);
        var rows = table.getElementsByTagName('tr');
        var rowCount = rows.length;
        for (var x=1; x<rowCount; x++) {
            if (rows[x].getAttribute("id")!=="0") {
                rows[x].setAttribute("status", "inactive");
            }
        }
    } catch(err) {
        alert(err.message);
    }
}

function cleanCurrentList(tableName) {
    try {
        var table = document.getElementById(tableName);
        var rows = table.getElementsByTagName('tr');
        var rowCount = rows.length;
        for (var x=rowCount-1; x>=0; x--) {
            if (rows[x].getAttribute("id")!=="0") {
                if (rows[x].getAttribute("status")==="inactive") {
                    table.deleteRow(x);
                }
            }
        }
    } catch(err) {
        alert(err.message);
    }
}

function loadCurrent() {
    var xmlhttp = new XMLHttpRequest();
    var url = "/current";
    try {
        xmlhttp.open("POST", url, false);
        xmlhttp.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
        xmlhttp.send("");
        obj = JSON.parse(xmlhttp.responseText);
        if (obj) {
            var table = document.getElementById("currentTable");
            var tableRows = table.getElementsByTagName('tr');
            var rowCount = tableRows.length;
            cleanCurrent("currentTable");
            for (i = 0; i < obj.sensor.length; i++) {
                var row = document.getElementById("row"+obj.sensor[i].location+
                                                            obj.sensor[i].type);
                if (row === null) {
                    row = table.insertRow(-1);
                    row.setAttribute("id", "row"+obj.sensor[i].location+
                                                            obj.sensor[i].type);
                    var cellLoc = row.insertCell(0);
                    var cellType = row.insertCell(1);
                    var cellValue = row.insertCell(2);

                    cellLoc.innerHTML = obj.sensor[i].location;
                    cellType.innerHTML = obj.sensor[i].type;
                    cellValue.innerHTML = obj.sensor[i].value;
                    row.setAttribute("status", "active");
                } else {
                    row.cells[0].innerHTML = obj.sensor[i].location;
                    row.cells[1].innerHTML = obj.sensor[i].type;
                    row.cells[2].innerHTML = obj.sensor[i].value;
                    row.setAttribute("status", "active");
                }
            }
        }
        cleanCurrentList("currentTable");
    } catch(err) {
        //alert(err.message);
    }
}

function onLoadCurrent() {
    clearInterval(mainInterval);
    loadCurrent();
    mainInterval=setInterval(function () {loadCurrent();}, 2000);
}
