(function () {
  const navLinks = document.querySelectorAll('nav a');
  const sections = [];

  navLinks.forEach(function (link) {
    const href = link.getAttribute('href');
    if (href && href.startsWith('#')) {
      const id = href.slice(1);
      const el = document.getElementById(id);
      if (el) sections.push({ el: el, link: link });
    }
  });

  function updateActive() {
    var current = null;
    var scrollY = window.scrollY + 80;

    for (var i = 0; i < sections.length; i++) {
      var sec = sections[i];
      if (sec.el.offsetTop <= scrollY) {
        current = sec.link;
      }
    }

    for (var j = 0; j < navLinks.length; j++) {
      navLinks[j].classList.remove('active');
    }
    if (current) current.classList.add('active');
  }

  window.addEventListener('scroll', updateActive);
  window.addEventListener('resize', updateActive);
  updateActive();
})();
