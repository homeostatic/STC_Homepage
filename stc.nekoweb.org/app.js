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

function closeSidebar() {
    if (!sidebar.classList.contains("close")) {
        toggleSidebar();
    }
}
function openSidebar() {
    if (sidebar.classList.contains("close")) {
        toggleSidebar();
    }
}

function toggleSubMenu(button) {
    if(sidebar.classList.contains("close")) {
        toggleSidebar();
    }
    button.nextElementSibling.classList.toggle("show");
    button.classList.toggle("rotate");
    // persistence between pages
    const subMenu = button.nextElementSibling;
    if (subMenu.classList.contains("show")) {
        localStorage.setItem(button.id, "yes");
    }else {
        localStorage.setItem(button.id, "no");
    }
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

// modal handling
function openModal(event) {
    const modal = document.getElementById("modal");
    const modalContent = document.getElementById('modal-content');
    const modalImg = event.target.cloneNode(true);
    modalContent.appendChild(modalImg);
    modal.classList.add("show");
}
function closeModal() {
    const modal = document.getElementById("modal");
    modal.classList.remove("show");
    const modalContent = document.getElementById('modal-content');
    modalContent.removeChild(modalContent.lastElementChild); // Clear the modal content
}
// modal event listeners
document.querySelectorAll('img').forEach(pic => {
    pic.addEventListener('click', openModal);
    });


//animate splash screen
function slideOut(element) {
    element.classList.add('slide-out');
}

function highlightActivePage() {
    const links = document.querySelectorAll('#sidebar a');
    links.forEach(link => {
    if (link.href === window.location.href) {
        link.parentElement.classList.add('active');
        
    }
    });
}

// Initialize sidebar
fetch('sidebar.html')
        .then(res => res.text())
        .then(sidebarhtml => {
        document.getElementById("sidebar-container").innerHTML = sidebarhtml;
        toggleButton= document.getElementById("toggle-btn");
        sidebar = document.getElementById("sidebar");
        highlightActivePage();

        // if state is saved, use it.
        sideVisible = localStorage.getItem("sideBarOpen") == "yes" ? true : false;
        if (!sideVisible) {
            closeSidebar();
        }else {
            //openSidebar(); // this is not needed, as the sidebar is open by default
            // load persistent submenu states, only relavant if sidebar is open
            const subMenus = document.querySelectorAll('.dropdown-btn');
            subMenus.forEach((button) => {localStorage.getItem(button.id) === "yes" ? toggleSubMenu(button):null ;});
        }
        const smallScreen = window.matchMedia("(max-width: 800px)");
        if (smallScreen.matches) {
            // if on small screen, close sidebar on load
            closeSidebar();
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







