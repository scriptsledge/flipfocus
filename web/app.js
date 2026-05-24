const firebaseConfig = {
    apiKey: "__FIREBASE_API_KEY__",
    authDomain: "flipfocus-iot.firebaseapp.com",
    databaseURL: "https://flipfocus-iot-default-rtdb.asia-southeast1.firebasedatabase.app",
    projectId: "flipfocus-iot",
    storageBucket: "flipfocus-iot.firebasestorage.app",
    messagingSenderId: "622810947515",
    appId: "1:622810947515:web:3f8d33646c994e1e45ee9e"
};

firebase.initializeApp(firebaseConfig);
const database = firebase.database();
let currentListenerRef = null;

function formatTime(seconds) {
    if (seconds === undefined || seconds === null) {
        return '<span class="val">0</span><span class="lbl">s</span>';
    }

    const sec = parseInt(seconds, 10);
    if (isNaN(sec) || sec <= 0) {
        return '<span class="val">0</span><span class="lbl">s</span>';
    }

    const h = Math.floor(sec / 3600);
    const m = Math.floor((sec % 3600) / 60);
    const s = sec % 60;

    let html = "";
    if (h > 0) {
        html += `<span class="val">${h}</span><span class="lbl">h</span>`;
    }
    if (m > 0) {
        html += `<span class="val">${m}</span><span class="lbl">m</span>`;
    }
    if (s > 0 || (h === 0 && m === 0)) {
        html += `<span class="val">${s}</span><span class="lbl">s</span>`;
    }

    return html;
}

function updateCardMetrics(nodeData) {
    document.getElementById('time-1').innerHTML = formatTime(nodeData.assignment);
    document.getElementById('time-2').innerHTML = formatTime(nodeData.examprep);
    document.getElementById('time-3').innerHTML = formatTime(nodeData.corecoding);
    document.getElementById('time-4').innerHTML = formatTime(nodeData.totalbreak);
}

function highlightActiveCard(activeZoneIndex) {
    for (let i = 1; i <= 4; i++) {
        document.getElementById(`card-${i}`).classList.remove('active');
    }

    if (activeZoneIndex) {
        const targetCard = document.getElementById(`card-${activeZoneIndex}`);
        if (targetCard) targetCard.classList.add('active');
    }
}

function viewModeChanged() {
    const selector = document.getElementById('time-frame');
    const choice = selector.value;
    const statusIndicator = document.getElementById('status-indicator');
    const statusText = document.getElementById('status-text');

    if (currentListenerRef) {
        currentListenerRef.off();
        currentListenerRef = null;
    }

    if (choice === 'live') {
        statusIndicator.className = 'status-dot';
        statusText.innerHTML = 'Mode: <strong>Live Tracking Stream</strong>';

        currentListenerRef = database.ref('/profiles');
        currentListenerRef.on('value', (snapshot) => {
            const data = snapshot.val();
            if (data) {
                updateCardMetrics({
                    assignment: data.assignment,
                    examprep: data.examprep,
                    corecoding: data.corecoding,
                    totalbreak: data.totalbreak
                });
                highlightActiveCard(data.activeZone);
            }
        });
    } else {
        statusIndicator.className = 'status-dot historical';
        statusText.innerHTML = `Viewing Log: <strong>Archive (${choice})</strong>`;

        currentListenerRef = database.ref(`/profiles/history/${choice}`);
        currentListenerRef.on('value', (snapshot) => {
            const data = snapshot.val();
            if (!data) {
                updateCardMetrics({ assignment: 0, examprep: 0, corecoding: 0, totalbreak: 0 });
            } else {
                updateCardMetrics(data);
            }
            highlightActiveCard(null);
        });
    }
}

window.addEventListener('DOMContentLoaded', () => {
    const selector = document.getElementById('time-frame');
    selector.addEventListener('change', viewModeChanged);
    viewModeChanged();
});
