// will be set after sidebar is injected
let toggleButton; 
let sidebar;
// will be set after buttons are injected
let enButton; 
let deButton;
let sideVisible = true; // default state of sidebar


function toggleSidebar() {

    sidebar = document.getElementById("sidebar");
    sidebar.classList.toggle("close");
    toggleButton.classList.toggle("rotate");

    // persistence between pages
    sideVisible = sidebar.classList.contains("close") ? false : true;
    localStorage.setItem("sideBarOpen", sideVisible ? "yes" : "no");

    Array.from(sidebar.getElementsByClassName("show")).forEach((ul) => {
        ul.classList.remove("show");
        ul.previousElementSibling.classList.remove("rotate");
    })
}        

function toggleSubMenu(button) {
    if(sidebar.classList.contains("close")) {
        toggleSidebar();
    }
    button.nextElementSibling.classList.toggle("show");
    button.classList.toggle("rotate");


}

// language toggle
function LangEn(){
    for(let en of document.querySelectorAll('[lang="en"]')){
        en.style.display = "inline";
    }
    for(let de of document.querySelectorAll('[lang="de"]')){
        de.style.display = "none";
    }

}
function LangDe(){
    for(let de of document.querySelectorAll('[lang="de"]')){
        de.style.display = "inline";
    }
    for(let en of document.querySelectorAll('[lang="en"]')){
        en.style.display = "none";
    }
}

function updateLanguage(lang) {
    if (lang === "en") {
        enButton.classList.add("active");
        deButton.classList.remove("active");
        localStorage.setItem("language", "en");
        LangEn();
    } else if (lang === "de") {
        deButton.classList.add("active");
        enButton.classList.remove("active");
        localStorage.setItem("language", "de");
        LangDe();
    }
}


//animate splash screen
function slideOut(element) {
    element.classList.add('slide-out');
}

// Initialize sidebar
fetch('sidebar.html')
        .then(res => res.text())
        .then(sidebarhtml => {
        document.getElementById("sidebar-container").innerHTML = sidebarhtml;
        toggleButton= document.getElementById("toggle-btn");
        highlightActivePage();
        // if state is saved, use it.
        sideVisible = localStorage.getItem("sideBarOpen") == "yes" ? true : false;
        if (!sideVisible) {
            toggleSidebar();
        }
   

        // initialise language select
        fetch('lang.html')
            .then(res => res.text())
            .then(langhtml => {
                document.getElementById("lang-container").innerHTML = langhtml;
                enButton = document.getElementById("en-btn");
                deButton = document.getElementById("de-btn");
                // Set initial language based on localStorage or default to English
                const initialLanguage = localStorage.getItem("language") || "en";
                //this should hide the elements that are not in the initial language, import that it is called after the everything is loaded
                updateLanguage(initialLanguage);
            });
        });

    function highlightActivePage() {
        const links = document.querySelectorAll('#sidebar a');
        links.forEach(link => {
        if (link.href === window.location.href) {
            link.parentElement.classList.add('active');
            
        }
        });
    }




