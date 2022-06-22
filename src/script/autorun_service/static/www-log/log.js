//const ws = new WebSocket('ws://localhost:9595/');
//var queue = [];
//var reader = new FileReader();
//reader.onload = function(event){
//    queue.push(reader.result)//内容就在这里
//};

window.onload = (event) => {
    loopFun(3000,getLog)
    loopFun(1500,getSummary)


/*    
    ws.onopen = function() {
        console.log('WebSocket Client Connected');
        ws.send('Hi this is web client.');
    };
    ws.onmessage = function(event) {
              reader.readAsText(event.data);            
   }

   ws.onclose = function(event) {

  }; 
 */ 
    document.querySelectorAll(".menu").forEach(item => {
  
      if (item.hasAttribute('data-checked')) {
        ref = item.getAttribute("data-ref")
        document.getElementById(ref).setAttribute('style', 'display:block')
      }
  
      item.addEventListener("click", (e) => {
        document.querySelectorAll(".menu").forEach(x => { x.removeAttribute("data-checked") })
        e.currentTarget.setAttribute("data-checked", "")
        document.querySelectorAll(".jumbotron-area").forEach(x => { x.setAttribute('style', 'display:none') })
        ref = e.currentTarget.getAttribute("data-ref")
        document.getElementById(ref).setAttribute('style', 'display:block')
      })
    })
  }
  
  
  
  function loopFun(delay,fun) {
    fun()
    setInterval(fun, delay);
  }
  
  function getLog() {
/*
    queue.forEach(text =>
        document.getElementById('log').innerText += text
    )
    
    var checkbox = document.getElementById('auto')
    if (checkbox.checked === true)
        document.getElementById("log").scrollBy(0, document.getElementById("log").scrollHeight)
 */
    
    
    let xhttp = new XMLHttpRequest()
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("log").innerHTML = this.responseText
        var checkbox = document.getElementById('auto')
        if (checkbox.checked === true)
          document.getElementById("log").scrollBy(0, document.getElementById("log").scrollHeight)
      }
    }
    xhttp.open("GET", "log", true)
    xhttp.send()
  }
  
  function getReport() {
    let xhttp = new XMLHttpRequest()
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        let data = JSON.parse(this.responseText)
        document.getElementById("wrapper").innerHTML =""      
        var tree = jsonTree.create(data, document.getElementById("wrapper"))
        tree.expand(true)
      }
    }
    xhttp.open("GET", "report", true)
    xhttp.send()
  }
  
  function getAllPassTree() {
    let xhttp = new XMLHttpRequest()
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        let data = JSON.parse(this.responseText)
        document.getElementById("show_tree").innerHTML ="" 
        var tree = jsonTree.create(data, document.getElementById("show_tree"))
  
      }
    }
    xhttp.open("GET", "all_pass_tree", true)
    xhttp.send()
  }
  
  function getSummary() {
    let xhttp = new XMLHttpRequest()
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
          let summary = JSON.parse(this.responseText)
  
          if(summary.stage!=="grid" && summary.stage!=="clip")
          {
            document.querySelectorAll(".summary_state").forEach(x => { 
              if(document.getElementById("nothing") !== x)
                  x.removeAttribute("data-checked") 
              else
                  x.setAttribute("data-checked", "")
            })
            document.getElementById("tbody_summary").innerHTML =""
          }
  
          if(summary.stage==="grid")
          {
            document.querySelectorAll(".summary_state").forEach(x => { 
              if(document.getElementById("gridding") !== x)
                  x.removeAttribute("data-checked") 
              else
                  x.setAttribute("data-checked", "")
            })
  
            let table = summary.list.map( item =>{
              return `<tr><td>${item.name}</td><td data-state="${item.state}">${item.state}</td></tr>`
            })
  
            document.getElementById("tbody_summary").innerHTML = table.join('')
          }
  
          if(summary.stage==="clip")
          {
            document.querySelectorAll(".summary_state").forEach(x => { 
              if(document.getElementById("clipping") !== x)
                  x.removeAttribute("data-checked") 
              else
                  x.setAttribute("data-checked", "")
            })
  
            let table = summary.list.map( item =>{
              return `<tr><td>${item.name}</td><td data-state="${item.state}">${item.state}</td></tr>`
            })
            table
            document.getElementById("tbody_summary").innerHTML = table.join('')
          }
      
      }
    }
    xhttp.open("GET", "run_summary", true);
    xhttp.send();
  }
  
  
  