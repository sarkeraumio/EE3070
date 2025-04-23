import pandas as pd
from sklearn.ensemble import IsolationForest
import requests

# ThingSpeak credentials
CHANNEL_ID = "YOUR_CHANNEL_ID"
READ_API_KEY = "YOUR_READ_API_KEY"
WRITE_API_KEY = "YOUR_WRITE_API_KEY"

# Step 1: Fetch data
url = f"https://api.thingspeak.com/channels/{CHANNEL_ID}/feeds.json?api_key={READ_API_KEY}&results=50"
r = requests.get(url)
data = pd.DataFrame(r.json()["feeds"])

# Step 2: Prepare data
data["voltage"] = pd.to_numeric(data["field1"], errors="coerce")
data["power"] = pd.to_numeric(data["field2"], errors="coerce")
data.dropna(subset=["voltage", "power"], inplace=True)

# Step 3: Run Isolation Forest
model = IsolationForest(contamination=0.1, random_state=42)
data["anomaly"] = model.fit_predict(data[["voltage", "power"]])

# Step 4: Detect latest status
latest_anomaly = int(data.iloc[-1]["anomaly"] == -1)

# Step 5: Push to ThingSpeak (field5 = anomaly)
update_url = f"https://api.thingspeak.com/update?api_key={WRITE_API_KEY}&field5={latest_anomaly}"
requests.get(update_url)
