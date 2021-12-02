function openTab(evt, tabName) {
    console.log(evt);
    // Declare all variables
    var i, tabcontent, tablinks;
  
    // Get all elements with class="tabcontent" and hide them
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) {
      tabcontent[i].style.display = "none";
    }
  
    // Get all elements with class="tabcontent" and remove the class "active"
    //tablinks = document.getElementsByClassName("tabcontent");
    //for (i = 0; i < tablinks.length; i++) {
      //tablinks[i].className = tablinks[i].className.replace(" pure-menu-selected", "");
    //  tablinks[i].classList.remove("pure-menu-selected")
   // }
  
    // Show the current tab, and add an "pure-menu-selected" class to the object that opened the tab
    document.getElementById(tabName).style.display = "block";
    //evt.path[1].className += " pure-menu-selected";
    //evt.currentTarget.className += " pure-menu-selected";
  }